struct Order {
    std::uint32_t stock_locate;
    std::uint32_t tracking_number; // Nasdaq internal tracking number
    std::uint32_t timestamp;
    std::uint64_t order_id; // order reference number
};

/*
Stock Locate 1 2 Integer Locate code identifying the security
Tracking Number 3 2 Integer Nasdaq internal tracking number
Timestamp 5 6 Integer Nanoseconds since midnight.
Order Reference
Number
11 8 Integer The unique reference number assigned to the new order at
the time ofreceipt.
Buy/Sell Indicator 19 1 Alpha The type of order being added.
“B” =Buy Order. “S” = SellOrder.
Shares 20 4 Integer The total number ofshares associated with the order being
added to the book.
Stock 24 8 Alpha Stock symbol, right padded with spaces
Price 32 4 Price (4) The display price of the new order. Refer to Data Types for
field processing notes. */
struct AddOrder : public Order {
    std::uint32_t price; // price
    std::uint8_t side; // buy/sell indicator
    std::uint32_t shares; // total number of shares associated with the order being added to the book
};

struct AddOrderWithMPID : public Order {
    std::uint8_t mmpid[4]; // Attribution 36 4 Alpha Nasdaq Market participant identifier associated with the object
};

/*
Order Executed Message
Name Offset Length Value Notes
Message Type 0 1 “E” Add Order – Order Executed Message
Stock Locate 1 2 Integer Locate code identifying the security
Tracking Number 3 2 Integer Nasdaq internal tracking number
Timestamp 5 6 Integer Nanoseconds since midnight
Order Reference
Number
11 8 Integer The unique reference number assigned to the new order at
the time ofreceipt
Executed Shares 19 4 Integer The number of shares executed
Match Number 23 8 Integer The Nasdaq generated day unique Match Number of this
execution. The Match Number is also referenced in the
Trade Break Message
*/
struct ExecuteOrder : public Order {
    std::uint64_t match_number; // TODO resize to 8 bytes 
    std::uint32_t shares; // number of shares executed
};

/*
Printable 31 1 Alpha Indicates if the execution should be reflected on time and
sales displays and volume calculations
“N” = Non-Printable
“Y” = Printable
Execution Price 32 4 Price(4) The Price at which the order execution occurred. Refer to
Data Types for field processing notes
*/
struct ExecuteOrderWithPrice : public ExecuteOrder {
    std::uint8_t printable; // Indicates if the execution should be reflected on time and sales displays and volume calculations
    std::uint32_t price;
};

struct CancelOrder : public Order {
    std::uint32_t shares; // number of shares canceled
};

/*
Order Delete Messa
Name
ge
Offset Length Value Notes
Message Type 0 1 “D” Order Delete Message
Stock Locate 1 2 Integer Locate code identifying the security
Tracking Number 3 2 Integer Nasdaq internal tracking number
Timestamp 5 6 Integer Nanoseconds since midnight
Order Reference
Number
11 8 Integer The reference number of the order being canceled*/
struct DeleteOrder : public Order {};

/*
Name Offset Length Value Notes
Message Type 0 1 “U” Order Replace Message
Stock Locate 1 2 Integer Locate code identifying the security
Tracking Number 3 2 Integer Nasdaq internal tracking number
Timestamp 5 6 Integer Nanoseconds since midnight
Original Order
Reference Number
11 8 Integer The original order reference number of the order being
replaced
New Order
Reference Number
19 8 Integer The new reference number for this order at time of
replacement
Please note that the Nasdaq system will use this new
order reference number for all subsequent updates
Shares 27 4 Integer The new total displayed quantity
Price 31 4 Price (4) The new display price for the order
Please refer to Data Types for field processing notes*/
struct ReplaceOrder : public Order {
    std::uint64_t new_order_id; // The unique reference number assigned to the new order at the time of receipt
    std::uint32_t shares; // The total number of shares associated with the order being added to the book
    std::uint32_t price; // The display price of the new order. Refer to Data Types for field processing notes.
};

namespace parser {
    Order parse_order(std::uint8_t* ptr, std::size_t length) {
        // 1 byte message type
        auto type = static_cast<char>(ptr[0]);
        switch (type) {
            case 'A':
                return *reinterpret_cast<AddOrder*>(ptr);
            case 'F':
                return *reinterpret_cast<AddOrderWithMPID*>(ptr);
            case 'E':
                return *reinterpret_cast<ExecuteOrder*>(ptr);
            case 'C':
                return *reinterpret_cast<ExecuteOrderWithPrice*>(ptr);
            case 'X':
                return *reinterpret_cast<CancelOrder*>(ptr);
            case 'D':
                return *reinterpret_cast<DeleteOrder*>(ptr);
            case 'U':
                return *reinterpret_cast<ReplaceOrder*>(ptr);
            default:
                throw std::runtime_error("Unknown message type");
        }
    }
}