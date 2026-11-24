#pragma once
#include <cstdint>

namespace gt4 {
class VehiclePayment {
    int* wallet = nullptr;
    int reserved = 0;
    bool completed = false;
public:
    VehiclePayment() = default;
    VehiclePayment(const VehiclePayment&) = delete;
    VehiclePayment& operator=(const VehiclePayment&) = delete;
    ~VehiclePayment() { cancel(); }

    bool reserve(int* balance, int price, int otherCosts)
    {
        if (wallet || completed || price < 0 || otherCosts < 0) return false;
        const int64_t total = int64_t(price) + otherCosts;
        if (total && (!balance || int64_t(*balance) < total)) return false;
        wallet = balance;
        reserved = price;
        if (reserved) *wallet -= reserved;
        return true;
    }

    void commit()
    {
        completed = true;
        reserved = 0;
        wallet = nullptr;
    }

    void cancel()
    {
        if (wallet && reserved) *wallet += reserved;
        reserved = 0;
        wallet = nullptr;
    }
};
}
