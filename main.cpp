#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"
#include <cstdlib>

using namespace std;
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string sendRequest(const string& url, const json& payload, const string& accessToken = "") {
    string readBuffer;
    CURL* curl;
    CURLcode res;

    try {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);

            string jsonStr = payload.dump();
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            if (!accessToken.empty()) {
                headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                cerr << "Request failed: " << curl_easy_strerror(res) << endl;
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
        }

        curl_global_cleanup();
    } catch (const exception& e) {
        cerr << "Exception in sendRequest: " << e.what() << endl;
    }

    return readBuffer;
}

string getAccessToken(const string& clientId, const string& clientSecret) {
    try {
        json payload = {
            {"id", 0},
            {"method", "public/auth"},
            {"params", {
                {"grant_type", "client_credentials"},
                {"scope", "session:apiconsole-c5i26ds6dsr expires:2592000"},
                {"client_id", clientId},
                {"client_secret", clientSecret}
            }},
            {"jsonrpc", "2.0"}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/public/auth", payload);
        auto responseJson = json::parse(response);

        if (responseJson.contains("result") && responseJson["result"].contains("access_token")) {
            return responseJson["result"]["access_token"];
        } else {
            cerr << "Failed to retrieve access token." << endl;
        }
    } catch (const exception& e) {
        cerr << "Exception in getAccessToken: " << e.what() << endl;
    }

    return "";
}

void placeOrder(const string& price, const string& accessToken, const string& amount, const string& instrument) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/buy"},
            {"params", {
                {"instrument_name", instrument},
                {"type", "limit"},
                {"price", price},
                {"amount", amount}
            }},
            {"id", 1}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/private/buy", payload, accessToken);
        cout << "Place Order Response: " << response << endl;
    } catch (const exception& e) {
        cerr << "Exception in placeOrder: " << e.what() << endl;
    }
}

void cancelOrder(const string& accessToken, const string& orderID) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/cancel"},
            {"params", {{"order_id", orderID}}},
            {"id", 6}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/private/cancel", payload, accessToken);
        cout << "Cancel Order Response: " << response << endl;
    } catch (const exception& e) {
        cerr << "Exception in cancelOrder: " << e.what() << endl;
    }
}

void modifyOrder(const string& accessToken, const string& orderID, int amount, double price) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/edit"},
            {"params", {
                {"order_id", orderID},
                {"amount", amount},
                {"price", price}
            }},
            {"id", 11}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/private/edit", payload, accessToken);
        cout << "Modify Order Response: " << response << endl;
    } catch (const exception& e) {
        cerr << "Exception in modifyOrder: " << e.what() << endl;
    }
}

void getPosition(const string& accessToken, const string& instrument) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_position"},
            {"params", {{"instrument_name", instrument}}},
            {"id", 20}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/private/get_position", payload, accessToken);
        auto responseJson = json::parse(response);

        if (responseJson.contains("result")) {
            auto result = responseJson["result"];
            cout << "Position Details for " << instrument << ":\n";
            cout << "Estimated Liquidation Price: " << result["estimated_liquidation_price"] << '\n';
            cout << "Size Currency: " << result["size_currency"] << '\n';
            cout << "Total Profit Loss: " << result["total_profit_loss"] << '\n';
            cout << "Leverage: " << result["leverage"] << '\n';
            cout << "Average Price: " << result["average_price"] << '\n';
            cout << "Mark Price: " << result["mark_price"] << '\n';
        } else {
            cerr << "Error: Could not retrieve position data." << endl;
        }
    } catch (const exception& e) {
        cerr << "Exception in getPosition: " << e.what() << endl;
    }
}

void getOrderBook(const string& accessToken, const string& instrument) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "public/get_order_book"},
            {"params", {{"instrument_name", instrument}}},
            {"id", 15}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/public/get_order_book", payload, accessToken);

        try {
            auto responseJson = json::parse(response);

            if (responseJson.contains("result")) {
                cout << "Order Book for " << instrument << ":\n";
                cout << "Best Bid Price: " << responseJson["result"]["best_bid_price"] 
                     << ", Amount: " << responseJson["result"]["best_bid_amount"] << '\n';
                cout << "Best Ask Price: " << responseJson["result"]["best_ask_price"] 
                     << ", Amount: " << responseJson["result"]["best_ask_amount"] << '\n';

                cout << "Asks:\n";
                for (const auto& ask : responseJson["result"]["asks"]) {
                    cout << "Price: " << ask[0] << ", Amount: " << ask[1] << '\n';
                }

                cout << "\nBids:\n";
                for (const auto& bid : responseJson["result"]["bids"]) {
                    cout << "Price: " << bid[0] << ", Amount: " << bid[1] << '\n';
                }

                cout << "\nMark Price: " << responseJson["result"]["mark_price"] << '\n';
                cout << "Open Interest: " << responseJson["result"]["open_interest"] << '\n';
                cout << "Timestamp: " << responseJson["result"]["timestamp"] << '\n';
            } else {
                cerr << "Error: Could not retrieve order book data." << endl;
            }
        } catch (const json::exception& e) {
            cerr << "JSON Parsing Error: " << e.what() << endl;
        }
    } catch (const exception& e) {
        cerr << "Exception in getOrderBook: " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error occurred in getOrderBook." << endl;
    }
}


void getOpenOrders(const string& accessToken) {
    try {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_open_orders"},
            {"params", {{"kind", "future"}, {"type", "limit"}}},
            {"id", 25}
        };

        string response = sendRequest("https://test.deribit.com/api/v2/private/get_open_orders", payload, accessToken);

        try {
            auto responseJson = json::parse(response);

            if (responseJson.contains("result")) {
                cout << "Open Orders:\n";
                for (const auto& order : responseJson["result"]) {
                    cout << "Instrument: " << order["instrument_name"] << ", Order ID: " << order["order_id"]
                        << ", Price: " << order["price"] << ", Amount: " << order["amount"] << '\n';
                }
            } else {
                cerr << "Error: Could not retrieve open orders." << endl;
            }
        } catch (const json::exception& e) {
            cerr << "JSON Parsing Error: " << e.what() << endl;
        }
    } catch (const exception& e) {
        cerr << "Exception in getOpenOrders: " << e.what() << endl;
    } catch (...) {
        cerr << "Unknown error occurred in getOpenOrders." << endl;
    }
}

int main() {
    const char* clientId = getenv("CLIENT_ID");
    const char* clientSecret = getenv("CLIENT_SECRET");

    if (!clientId || !clientSecret) {
        cerr << "Error: CLIENT_ID or CLIENT_SECRET environment variables are not set." << endl;
        return 1;
    }

    try {
        string accessToken = getAccessToken(clientId, clientSecret);
        if (!accessToken.empty()) {
            placeOrder("20", accessToken, "10","ETH-PERPETUAL");
            placeOrder("20", accessToken, "10","BTC-PERPETUAL");
            placeOrder("30", accessToken, "10","BTC-PERPETUAL");
            cancelOrder(accessToken,"ETH-21668794289");
            modifyOrder(accessToken,"ETH-21668794289",30,30);
            getOrderBook(accessToken,"BTC-PERPETUAL");
            getPosition(accessToken, "BTC-PERPETUAL");
            getOpenOrders(accessToken);
        } else {
            cerr << "Unable to obtain access token." << endl;
        }
    } catch (const exception& e) {
        cerr << "Exception in main: " << e.what() << endl;
    }

    return 0;
}