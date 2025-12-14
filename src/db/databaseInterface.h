#pragma once
#include <vector>
#include <string>
#include <utility>

struct Transaction {
    int transaction_id;
    int sender_id;
    int receiver_id;
    double amount;
    std::string timestamp;
    std::string status;
};

class IDatabase {
public:
    virtual ~IDatabase() = default;

    virtual int TransferMoney(int sender_id, int receiver_id, double amount) = 0;
    virtual std::pair<double, bool> GetBalance(int user_id) = 0;
    virtual void DepositMoney(int sender_id, double amount) = 0;
    virtual int WithdrawMoney(int sender_id, double amount) = 0;
    virtual std::vector<Transaction> GetTransactions(int user_id) = 0;
};
