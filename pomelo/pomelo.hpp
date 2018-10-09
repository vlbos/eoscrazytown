#include <eosiolib/singleton.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/asset.hpp>
#include <math.h>
#include <string>
#include <vector>

#define EOS S(4, EOS)
#define TOKEN_CONTRACT N(eosio.token)

using namespace eosio;
using namespace std;

class pomelo : public contract
{
public:
    pomelo(account_name self) :
        contract(self) {
    }

    // @abi action
    void cancelsell(account_name account, string symbol, uint64_t id);

    // @abi action
    void cancelbuy(account_name account, string symbol, uint64_t id);

    // @abi action
    void setwhitelist(string symbol, account_name issuer);

    // @abi action
    void rmwhitelist(string symbol);

    void apply(account_name contract, action_name act);   

    void onTransfer(account_name from,
                    account_name to,
                    asset        quantity,
                    string       memo);  

    void transfer(account_name from,
                  account_name to,
                  asset        quantity,
                  string       memo);                 

    // @abi table buyorder i64
    struct buyorder { 
        uint64_t id;
        account_name account;
        asset bid;
        asset ask;
        uint64_t unit_price;
        time timestamp;

        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return unit_price; }
        EOSLIB_SERIALIZE(buyorder, (id)(account)(bid)(ask)(unit_price)(timestamp))
    };
    typedef eosio::multi_index<N(buyorder), buyorder, 
        indexed_by<N(byprice), const_mem_fun<buyorder, uint64_t, &buyorder::get_price>>
    > buyorders;

    // @abi table sellorder i64
    struct sellorder {
        uint64_t id;
        account_name account;
        asset bid;
        asset ask;
        uint64_t unit_price;
        time timestamp;

        uint64_t primary_key() const { return id; }
        uint64_t get_price() const { return unit_price; }
        EOSLIB_SERIALIZE(sellorder, (id)(account)(bid)(ask)(unit_price)(timestamp))
    };
    typedef eosio::multi_index<N(sellorder), sellorder, 
        indexed_by<N(byprice), const_mem_fun<sellorder, uint64_t, &sellorder::get_price>>
    > sellorders;
 
    // @abi table whitelist i64
    struct whitelist {
        account_name contract;
    };
    typedef singleton<N(whitelist), whitelist> whitelist_index;
    
    // @abi action
    void buyreceipt(buyorder o) {
        require_auth(_self);
    }    

    // @abi action
    void sellreceipt(sellorder t) {
        require_auth(_self);
    }   

private:
    uint64_t my_string_to_symbol(uint8_t precision, const char* str);
    bool is_valid_unit_price(uint64_t eos, uint64_t non_eos);
    void assert_whitelist(string symbol, account_name contract);
    void assert_whitelist(symbol_type symbol, account_name contract);
    uint64_t string_to_amount(string s);
    vector<string> split(string src, char c);
    account_name get_contract_name_by_symbol(string symbol);
    account_name get_contract_name_by_symbol(symbol_type symbol);
    void publish_buyorder_if_needed(account_name account, asset bid, asset ask);
    void publish_sellorder_if_needed(account_name account, asset bid, asset ask);
    void buy(account_name account, asset bid, asset ask);
    void sell(account_name account, asset bid, asset ask);    
};

void pomelo::apply(account_name contract, action_name act) 
{
    auto &thiscontract = *this;
    if (act == N(transfer)) {
        auto transfer = unpack_action_data<currency::transfer>();
        if (transfer.quantity.symbol == EOS) 
        {
            eosio_assert(contract == TOKEN_CONTRACT, "Transfer EOS must go through eosio.token");
        }
        else
        {
            assert_whitelist(transfer.quantity.symbol, contract);
        }
        onTransfer(transfer.from, transfer.to, transfer.quantity, transfer.memo);
        return;
    }

    if (contract != _self) return;

    switch (act) {
        EOSIO_API(pomelo, (cancelbuy)(cancelsell)(setwhitelist)(rmwhitelist))
    };
}

extern "C" {
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) 
    {
        pomelo p(receiver);
        p.apply(code, action);
        eosio_exit(0);
    }
}
