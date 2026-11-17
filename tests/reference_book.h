#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <fastbook/types.h>

namespace fastbook::test {

class ReferenceBook {
public:
    struct Entry {
        OrderID id;
        Side side;
        Price price;
        Quantity quantity;
    };

    struct LevelView {
        Price price = 0;
        Quantity total_quantity = 0;
        std::vector<std::pair<OrderID, Quantity>> queue;
    };

    bool add(OrderID id, Side side, Price price, Quantity quantity) {
        if (quantity == 0 || find(id) != orders_.end()) return false;
        orders_.push_back({id, side, price, quantity});
        return true;
    }

    bool reduce(OrderID id, Quantity quantity) {
        auto it = find(id);
        if (it == orders_.end() || it->quantity < quantity) return false;
        it->quantity -= quantity;
        if (it->quantity == 0) orders_.erase(it);
        return true;
    }

    bool remove(OrderID id) {
        auto it = find(id);
        if (it == orders_.end()) return false;
        orders_.erase(it);
        return true;
    }

    bool replace(OrderID old_id, OrderID new_id, Quantity quantity, Price price) {
        auto it = find(old_id);
        if (it == orders_.end()) return false;
        if (find(new_id) != orders_.end()) return false;
        if (quantity == 0) return false;
        Side side = it->side;
        orders_.erase(it);
        orders_.push_back({new_id, side, price, quantity});
        return true;
    }

    std::vector<LevelView> depth(Side side, std::size_t n) const {
        std::vector<Price> prices;
        for (const Entry& e : orders_) {
            if (e.side == side) prices.push_back(e.price);
        }
        std::sort(prices.begin(), prices.end());
        prices.erase(std::unique(prices.begin(), prices.end()), prices.end());
        if (side == Side::BUY) std::reverse(prices.begin(), prices.end());
        if (prices.size() > n) prices.resize(n);

        std::vector<LevelView> result;
        for (Price p : prices) {
            LevelView view;
            view.price = p;
            for (const Entry& e : orders_) {
                if (e.side == side && e.price == p) {
                    view.total_quantity += e.quantity;
                    view.queue.emplace_back(e.id, e.quantity);
                }
            }
            result.push_back(std::move(view));
        }
        return result;
    }

    std::vector<OrderID> live_ids() const {
        std::vector<OrderID> ids;
        for (const Entry& e : orders_) ids.push_back(e.id);
        return ids;
    }

private:
    std::vector<Entry>::iterator find(OrderID id) {
        return std::find_if(orders_.begin(), orders_.end(),
                            [id](const Entry& e) { return e.id == id; });
    }

    std::vector<Entry> orders_;
};

}  // namespace fastbook::test
