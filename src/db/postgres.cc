#include "postgres.h"

PostgresDatabase::PostgresDatabase(const std::string& conn_str) : conn(conn_str) {
    if (!conn.is_open()) {
        throw std::runtime_error("Failed to connect to database");
    }
}

int PostgresDatabase::TransferMoney(int sender_id, int receiver_id, double amount) {
    pqxx::work txn(conn);
    pqxx::result sender_balance_result = txn.exec_params(
    "SELECT balance FROM users WHERE user_id = $1", sender_id);
     pqxx::result receiver = txn.exec_params(
    "SELECT balance FROM users WHERE user_id = $1", receiver_id);
    if (sender_balance_result.size() == 0) {
        return 1; // sender not found
    }
    else if (receiver.size() == 0) {
        return 2; // receiver not found
    }
    double sender_balance = sender_balance_result[0][0].as<double>();
    if (sender_balance < amount) {
        return 3; //not enough money
    }

    txn.exec_params("UPDATE users SET balance = balance - $1 WHERE user_id = $2", amount, sender_id);
    txn.exec_params("UPDATE users SET balance = balance + $1 WHERE user_id = $2", amount, receiver_id);

    txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'transfer')",
                                               sender_id, receiver_id, amount);

    txn.commit();
    return 0;
}

std::pair<double, bool> PostgresDatabase::GetBalance(int user_id) {
    pqxx::work txn(conn);
    pqxx::result sender_balance_result = txn.exec_params(
    "SELECT balance FROM users WHERE user_id = $1", user_id);
    
    if (sender_balance_result.empty()) {
        return {-1, 1}; // user not found
    }

    double sender_balance = sender_balance_result[0][0].as<double>();
    return {sender_balance, 0};
}

void PostgresDatabase::DepositMoney(int user_id, double amount) {
    pqxx::work txn(conn);
    txn.exec_params("UPDATE users SET balance = balance + $1 WHERE user_id = $2", amount, user_id);

    txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'deposit')",
    user_id, user_id, amount);
    txn.commit();
}


int PostgresDatabase::WithdrawMoney(int user_id, double amount) {
    pqxx::work txn(conn);
    pqxx::result sender_balance_result = txn.exec_params(
    "SELECT balance FROM users WHERE user_id = $1", user_id);
    double sender_balance = sender_balance_result[0][0].as<double>();
    if (sender_balance < amount) {
        return 1;
    }
    else if (amount <= 0) {
        return 1;
    }
    else{
        txn.exec_params("UPDATE users SET balance = balance - $1 WHERE user_id = $2", amount, user_id);

        txn.exec_params("INSERT INTO transactions (sender_id, receiver_id, amount, status) VALUES ($1, $2, $3, 'withdrawal')",
        user_id, user_id, amount);
        txn.commit();
    }
    return 0;
}


std::vector<Transaction> PostgresDatabase::GetTransactions(int user_id) {
    pqxx::read_transaction txn(conn);
    auto r = txn.exec_params(
        "SELECT * FROM transactions WHERE sender_id=$1 OR receiver_id=$1", user_id);

    std::vector<Transaction> out(r.size());

    for (auto const& row : r) {
        out.push_back({
            row["transaction_id"].as<int>(),
            row["sender_id"].as<int>(),
            row["receiver_id"].as<int>(),
            row["amount"].as<double>(),
            row["timestamp"].as<std::string>(),
            row["status"].as<std::string>()
        });
    }
    return out;
}
