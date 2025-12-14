#include <grpcpp/grpcpp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <string>
#include "proto/payment_service.grpc.pb.h"
#include "src/db/postgres.h"

using grpc::Server;
using grpc::ServerBuilder;
using std::pair;

void load_env(const std::string& filename = ".env") {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t equal_pos = line.find('=');
        if (equal_pos != std::string::npos) {
            std::string key = line.substr(0, equal_pos);
            std::string value = line.substr(equal_pos + 1);
            setenv(key.c_str(), value.c_str(), true);
        }
    }
}






class PaymentServiceImpl final : public payment::PaymentService::Service {
private:
    PostgresDatabase* db;
public:
    PaymentServiceImpl(PostgresDatabase* database) : db(database) {}
    grpc::Status TransferMoney(grpc::ServerContext* context, const payment::TransferRequest* request, payment::TransferResponse* response) override {
        int sender_id = request->sender_id();
        int receiver_id = request->receiver_id();
        double amount = request->amount();

        try {
            bool result = db->TransferMoney(sender_id, receiver_id, amount);
            if (result != 0) {
                if (result == 1) {
                    response->set_success(false);
                    response->set_message("Sender not found.");
                    return grpc::Status::OK;
                }
                else if (result == 2) {
                    response->set_success(false);
                    response->set_message("Receiver not found.");
                    return grpc::Status::OK;
                }
                else if (result == 3) {
                    response->set_success(false);
                    response->set_message("Not enough money.");
                    return grpc::Status::OK;
                }
            }
            response->set_success(true);
            response->set_message("Transfer successful.");
        }
        catch (const std::exception& e) {
            response->set_success(false);
            response->set_message("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }

    grpc::Status CheckBalance(grpc::ServerContext* context, const payment::BalanceRequest* request, payment::BalanceResponse* response) override {
        int sender_id = request->user_id();
        try {
            pair<double, bool> balance = db->GetBalance(sender_id);
            if (balance.second != 0) {
                response->set_message("User not found.");
                return grpc::Status::OK;
            }
            response->set_balance(balance.first);
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }

    grpc::Status GetTransactionHistory(grpc::ServerContext* context, const payment::HistoryRequest* request, payment::HistoryResponse* response) override {
        int sender_id = request->user_id();
        try {
            std::vector<Transaction> out = db->GetTransactions(sender_id);

            for (const auto& row : out) {
                payment::Transaction* transaction = response->add_transactions();
                transaction->set_transaction_id(row.transaction_id);
                transaction->set_sender_id(row.sender_id);
                transaction->set_receiver_id(row.receiver_id);
                transaction->set_amount(row.amount);
                transaction->set_timestamp(row.timestamp);
                transaction->set_status(row.status);
            }
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }

    grpc::Status DepositMoney(grpc::ServerContext* context, const payment::DepositRequest* request, payment::DepositResponse* response) override {
        int sender_id = request->user_id();
        double amount = request->amount();
        try {
            db->DepositMoney(sender_id, amount);
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }

    grpc::Status WithdrawMoney(grpc::ServerContext* context, const payment::WithdrawRequest* request, payment::WithdrawResponse* response) override {
        int sender_id = request->user_id();
        double amount = request->amount();
        try {
            int result = db->WithdrawMoney(sender_id, amount);
            if (result != 0) {
                if (result == 1) {
                    return grpc::Status::CANCELLED;
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }
};

void RunServer(PostgresDatabase* db) {
    std::string server_address("0.0.0.0:50051");
    PaymentServiceImpl service(db);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    load_env();
    std::string db_user = getenv("DB_USER");
    std::string db_pass = getenv("DB_PASSWORD");
    std::string db_name = getenv("DB_NAME");
    std::string db_host = getenv("DB_HOST");
    std::string db_port = getenv("DB_PORT");

    const std::string conn("user=" + db_user +
                            " password=" + db_pass +
                            " dbname=" + db_name +
                            " host=" + db_host +
                            " port=" + db_port);
    PostgresDatabase db(conn);
    RunServer(&db);
    return 0;
}