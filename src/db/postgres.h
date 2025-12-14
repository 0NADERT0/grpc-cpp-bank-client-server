#include "databaseInterface.h"
#include <pqxx/pqxx>

class PostgresDatabase : public IDatabase {
public:
    PostgresDatabase(const std::string& conn_str);
    std::pair<double, bool> GetBalance(int user_id) override;
    int TransferMoney(int sender_id, int receiver_id, double amount) override;
    void DepositMoney(int user_id, double amount) override;
    int WithdrawMoney(int user_id, double amount) override;
    std::vector<Transaction> GetTransactions(int user_id) override;

private:
    pqxx::connection conn;
};
