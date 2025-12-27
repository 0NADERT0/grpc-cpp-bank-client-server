#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/db/databaseInterface.h"

using ::testing::Return;
using ::testing::Throw;
using ::testing::_;


class MockDatabase : public IDatabase {
public:
    MOCK_METHOD(int, TransferMoney, (int sender_id, int receiver_id, double amount), (override));
    MOCK_METHOD((std::pair<double, bool>), GetBalance, (int user_id), (override));
    MOCK_METHOD(void, DepositMoney, (int user_id, double amount), (override));
    MOCK_METHOD(int, WithdrawMoney, (int user_id, double amount), (override));
    MOCK_METHOD((std::vector<Transaction>), GetTransactions, (int user_id), (override));
};

class DatabaseTest : public ::testing::Test {
protected:
    MockDatabase db;
};

//TransferMoney

//success = 0
TEST_F(DatabaseTest, TransferMoneySuccess) {
    EXPECT_CALL(db, TransferMoney(1, 2, 100.0))
    .WillOnce(Return(0));
    
    int result = db.TransferMoney(1, 2, 100.0);
    EXPECT_EQ(result, 0);
}

//sender not found = 1
TEST_F(DatabaseTest, TransferMoneySenderNotFound) {
    EXPECT_CALL(db, TransferMoney(999, 2, 100.0))
    .WillOnce(Return(1));
    
    int result = db.TransferMoney(999, 2, 100.0);
    EXPECT_EQ(result, 1);
}

//receiver not found = 2
TEST_F(DatabaseTest, TransferMoneyReceiverNotFound) {
    EXPECT_CALL(db, TransferMoney(1, 999, 100.0))
    .WillOnce(Return(2));
    
    int result = db.TransferMoney(1, 999, 100.0);
    EXPECT_EQ(result, 2);
}

//not enough money = 3
TEST_F(DatabaseTest, TransferMoneyInsufficientFunds) {
    EXPECT_CALL(db, TransferMoney(1, 2, 10000.0))
    .WillOnce(Return(3));
    
    int result = db.TransferMoney(1, 2, 10000.0);
    EXPECT_EQ(result, 3);
}

//db error
TEST_F(DatabaseTest, TransferMoneyDatabaseException) {
    EXPECT_CALL(db, TransferMoney(1, 2, 100.0))
    .WillOnce(Throw(std::runtime_error("Database connection failed")));
    
    EXPECT_THROW(db.TransferMoney(1, 2, 100.0), std::runtime_error);
}

//GetBalance


//success
TEST_F(DatabaseTest, GetBalanceSuccess) {
    std::pair<double, bool> expected = {1500.0, true};
    EXPECT_CALL(db, GetBalance(1))
    .WillOnce(Return(expected));
    
    auto result = db.GetBalance(1);
    EXPECT_TRUE(result.second);
    EXPECT_DOUBLE_EQ(result.first, 1500.0);
}

//user not found
TEST_F(DatabaseTest, GetBalanceUserNotFound) {
    std::pair<double, bool> expected = {0.0, false};
    EXPECT_CALL(db, GetBalance(999))
    .WillOnce(Return(expected));
    
    auto result = db.GetBalance(999);
    EXPECT_FALSE(result.second);
    EXPECT_DOUBLE_EQ(result.first, 0.0);
}

//db error
TEST_F(DatabaseTest, GetBalanceDatabaseException) {
    EXPECT_CALL(db, GetBalance(1))
    .WillOnce(Throw(std::runtime_error("Database connection failed")));
    
    EXPECT_THROW(db.GetBalance(1), std::runtime_error);
}

//GetTransactions


//success
TEST_F(DatabaseTest, GetTransactionsSuccess) {
    std::vector<Transaction> transactions = {
    {1, 1, 2, 100.0, "2025-01-01", "completed"},
    {2, 1, 3, 50.0, "2025-01-02", "completed"}
    };
    
    EXPECT_CALL(db, GetTransactions(1))
    .WillOnce(Return(transactions));
    
    auto result = db.GetTransactions(1);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].amount, 100.0);
    EXPECT_EQ(result[1].amount, 50.0);
}

//no transactions
TEST_F(DatabaseTest, GetTransactionsEmpty) {
    std::vector<Transaction> empty;
    
    EXPECT_CALL(db, GetTransactions(999))
    .WillOnce(Return(empty));
    
    auto result = db.GetTransactions(999);
    EXPECT_EQ(result.size(), 0);
}

//db error
TEST_F(DatabaseTest, GetTransactionsDatabaseException) {
    EXPECT_CALL(db, GetTransactions(1))
    .WillOnce(Throw(std::runtime_error("Database connection failed")));
    
    EXPECT_THROW(db.GetTransactions(1), std::runtime_error);
}

//DepositMoney

//check if function works
TEST_F(DatabaseTest, DepositMoneySuccess) {
    EXPECT_CALL(db, DepositMoney(1, 500.0));
    
    db.DepositMoney(1, 500.0);
}

//
TEST_F(DatabaseTest, DepositMoneyDatabaseException) {
    EXPECT_CALL(db, DepositMoney(1, 500.0))
    .WillOnce(Throw(std::runtime_error("Database error")));
    
    EXPECT_THROW(db.DepositMoney(1, 500.0), std::runtime_error);
}

//WithdrawMoney

//success = 0
TEST_F(DatabaseTest, WithdrawMoneySuccess) {
    EXPECT_CALL(db, WithdrawMoney(1, 200.0))
    .WillOnce(Return(0));
    
    int result = db.WithdrawMoney(1, 200.0);
    EXPECT_EQ(result, 0);
}

//not enough money = 1
TEST_F(DatabaseTest, WithdrawMoneyInsufficientFunds) {
    EXPECT_CALL(db, WithdrawMoney(1, 5000.0))
    .WillOnce(Return(1));
    
    int result = db.WithdrawMoney(1, 5000.0);
    EXPECT_EQ(result, 1);
}

//db error
TEST_F(DatabaseTest, WithdrawMoneyDatabaseException) {
    EXPECT_CALL(db, WithdrawMoney(1, 200.0))
    .WillOnce(Throw(std::runtime_error("Database error")));
    
    EXPECT_THROW(db.WithdrawMoney(1, 200.0), std::runtime_error);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}