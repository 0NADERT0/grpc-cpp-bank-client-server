#include <grpcpp/grpcpp.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <string>
#include "proto/payment_service.grpc.pb.h"
#include <pqxx/pqxx>

using grpc::Server;
using grpc::ServerBuilder;

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

std::string db_user = getenv("DB_USER");
std::string db_pass = getenv("DB_PASSWORD");
std::string db_name = getenv("DB_NAME");
std::string db_host = getenv("DB_HOST");
std::string db_port = getenv("DB_PORT");

pqxx::connection conn("user=" + db_user +
                        " password=" + db_pass +
                        " dbname=" + db_name +
                        " host=" + db_host +
                        " port=" + db_port);

//pqxx::work txn(conn); // Delete this

class PaymentServiceImpl final : public payment::PaymentService::Service {
    grpc::Status TransferMoney(grpc::ServerContext* context, const payment::TransferRequest* request, payment::TransferResponse* response) override {
        int sender_id = request->sender_id();
        int receiver_id = request->receiver_id();
        double amount = request->amount();

        try {
            pqxx::work txn(conn);
            pqxx::result sender_balance_result = txn.exec_params(
                "SELECT balance FROM users WHERE user_id = $1", sender_id);
            if (sender_balance_result.size() == 0) {
                response->set_success(false);
                response->set_message("Sender not found.");
                return grpc::Status::OK;
            }
            double sender_balance = sender_balance_result[0][0].as<double>();
            if (sender_balance < amount) {
                response->set_success(false);
                response->set_message("Not enough money.");
                return grpc::Status::OK;
            }

            txn.exec_params("UPDATE users SET balance = balance - $1 WHERE user_id = $2", amount, sender_id);
            txn.exec_params("UPDATE users SET balance = balance + $1 WHERE user_id = $2", amount, receiver_id);

            txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'transfer')",
            sender_id, receiver_id, amount);

            txn.commit();
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
            pqxx::work txn(conn);
            pqxx::result sender_balance_result = txn.exec_params(
                "SELECT balance FROM users WHERE user_id = $1", sender_id);
            double sender_balance = sender_balance_result[0][0].as<double>();
            response->set_balance(sender_balance);
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }

    grpc::Status GetTransactionHistory(grpc::ServerContext* context, const payment::HistoryRequest* request, payment::HistoryResponse* response) override {
        int sender_id = request->user_id();
        try {
            pqxx::read_transaction txn(conn);
            pqxx::result sender_transactions = txn.exec_params(
                "SELECT * FROM transactions WHERE sender_id = $1", sender_id);
            pqxx::result receiver_transactions = txn.exec_params(
                "SELECT * FROM transactions WHERE receiver_id = $1", sender_id);

            for (const auto& row : sender_transactions) {
                payment::Transaction* transaction = response->add_transactions();
                transaction->set_transaction_id(row["transaction_id"].as<int>());
                transaction->set_sender_id(row["sender_id"].as<int>());
                transaction->set_receiver_id(row["receiver_id"].as<int>());
                transaction->set_amount(row["amount"].as<double>());
                transaction->set_timestamp(row["timestamp"].as<std::string>());
                transaction->set_status(row["status"].as<std::string>());
            }

            for (const auto& row : receiver_transactions) {
                payment::Transaction* transaction = response->add_transactions();
                transaction->set_transaction_id(row["transaction_id"].as<int>());
                transaction->set_sender_id(row["sender_id"].as<int>());
                transaction->set_receiver_id(row["receiver_id"].as<int>());
                transaction->set_amount(row["amount"].as<double>());
                transaction->set_timestamp(row["timestamp"].as<std::string>());
                transaction->set_status(row["status"].as<std::string>());
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
            pqxx::work txn(conn);
            txn.exec_params("UPDATE users SET balance = balance + $1 WHERE user_id = $2", amount, sender_id);

            txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'deposit')",
            sender_id, sender_id, amount);
            txn.commit();
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
            pqxx::work txn(conn);
            pqxx::result sender_balance_result = txn.exec_params(
                "SELECT balance FROM users WHERE user_id = $1", sender_id);
            double sender_balance = sender_balance_result[0][0].as<double>();
            if (sender_balance < amount) {
                return grpc::Status::CANCELLED;
            }
            else if (amount <= 0) {
                return grpc::Status::CANCELLED;
            }
            else{
                txn.exec_params("UPDATE users SET balance = balance - $1 WHERE user_id = $2", amount, sender_id);

                txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'withdrawal')",
                sender_id, sender_id, amount);
                txn.commit();
            }
        }
        catch (const std::exception& e) {
            std::cerr << ("Database error: " + std::string(e.what()));
        }

        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    PaymentServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    load_env();
    RunServer();
    return 0;
}