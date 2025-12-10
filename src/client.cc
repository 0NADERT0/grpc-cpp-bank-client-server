#include <grpcpp/grpcpp.h>
#include "proto/payment_service.grpc.pb.h"

using grpc::Channel;

int personal_id = 1;

class PaymentClient{
public:
    explicit PaymentClient(std::shared_ptr<Channel> channel)
        : stub_(payment::PaymentService::NewStub(channel)) {}

    void Transfer(int sender_id, int receiver_id, double amount){
        payment::TransferRequest request;
        request.set_sender_id(sender_id);
        request.set_receiver_id(receiver_id);
        request.set_amount(amount);

        payment::TransferResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->TransferMoney(&context, request, &response);

        if (status.ok()){
            std::cout << "Transfer result: " << response.message() << std::endl;
        }
        else{
            std::cerr << "Operation failed: " << status.error_message() << std::endl;
        }
    }

    void CheckBalance(int sender_id){
        payment::BalanceRequest request;
        request.set_user_id(sender_id);

        payment::BalanceResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->CheckBalance(&context, request, &response);

        if (status.ok()){
            std::cout << "Current Balance: " << response.balance() << std::endl;
        }
        else{
            std::cerr << "Operation failed" << std::endl;
        }
    }

    void GetTransactionHistory(int sender_id){
        payment::HistoryRequest request;
        request.set_user_id(sender_id);

        payment::HistoryResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->GetTransactionHistory(&context, request, &response);
        if (status.ok()){
            std::cout << "Your history of transactions: " << std::endl;
            for (const payment::Transaction& transaction : response.transactions()) {
                std::cout << "Transaction ID: " << transaction.transaction_id() << std::endl;
                std::cout << "Sender ID: " << transaction.sender_id() << std::endl;
                std::cout << "Receiver ID: " << transaction.receiver_id() << std::endl;
                std::cout << "Amount: " << transaction.amount() << std::endl;
                std::cout << "Timestamp: " << transaction.timestamp() << std::endl;
                std::cout << "Status: " << transaction.status() << std::endl;
                std::cout << "-------------------------" << std::endl;
            }
        }
        else{
            std::cerr << "Operation failed: " << status.error_message() << std::endl;
        }
    }

    void DepositMoney(int sender_id, double amount){
        payment::DepositRequest request;
        request.set_user_id(sender_id);
        request.set_amount(amount);

        payment::DepositResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->DepositMoney(&context, request, &response);

        if (status.ok()){
            std::cout << "Successfully deposited : " << amount << std::endl;
        }
        else{
            std::cerr << "Operation failed" << std::endl;
        }
    }

    void WithdrawMoney(int sender_id, double amount){
        payment::WithdrawRequest request;
        request.set_user_id(sender_id);
        request.set_amount(amount);

        payment::WithdrawResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub_->WithdrawMoney(&context, request, &response);

        if (status.ok()){
            std::cout << "Successfully withdrew : " << amount << std::endl;
        }
        else{
            std::cerr << "Operation failed" << std::endl;
        }
    }

private:
    std::unique_ptr<payment::PaymentService::Stub> stub_;
};

int main(){
    PaymentClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    while(true){
        std::cout << "What would you want to do? Enter a number:" << std::endl;
        std::cout << "1: To see your balance" << std::endl;
        std::cout << "2: To transfer money to other person by id" << std::endl;
        std::cout << "3: To deposit money" << std::endl;
        std::cout << "4: To withdraw money" << std::endl;
        std::cout << "5: To see history of transactions" << std::endl;
        int command = 0;
        std::cin >> command;
        if (command == 1) {
            client.CheckBalance(personal_id);
        }
        else if (command == 2) {
            int user_id = -1;
            std::cout << "Type the id of user" << std::endl;
            std::cin >> user_id;
            double amount = 0;
            std::cout << "Input amount of money to transfer" << std::endl;
            std::cin >> amount;
            client.Transfer(personal_id, user_id, amount);
            client.CheckBalance(personal_id);
        }
        else if (command == 3) {
            double amount = 0;
            std::cout << "What amount of money to deposit: " << std::endl;
            std::cin >> amount;
            client.DepositMoney(personal_id, amount);
            client.CheckBalance(personal_id);
            
        }
        else if (command == 4) {
            client.CheckBalance(personal_id);
            double amount = 0;
            std::cout << "What amount of money to withdraw: " << std::endl;
            std::cin >> amount;
            client.WithdrawMoney(personal_id, amount);
            client.CheckBalance(personal_id);
        }
        else if (command == 5) {
            client.GetTransactionHistory(personal_id);
        }
    }
    return 0;
}