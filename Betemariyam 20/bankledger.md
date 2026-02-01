Bank Transaction Ledger with Rollback (C)
Files
bank_ledger.c (or whatever name you used)

Requirements
C compiler (e.g. gcc or clang)
Standard C library only (no extra dependencies)

Compile
gcc -Wall -Wextra -O2 -o bank_ledger bank_ledger.c
If you prefer clang:

clang -Wall -Wextra -O2 -o bank_ledger bank_ledger.c
Run
./bank_ledger

Commands (typed inside the program)
CREATE_ACCOUNT <id> <initial_balance>
Example:
CREATE_ACCOUNT A1 1000
TXN DEPOSIT <account> <amount>
TXN WITHDRAW <account> <amount>
TXN TRANSFER <from_account> <to_account> <amount>
Examples:
TXN DEPOSIT A1 200
TXN WITHDRAW A1 300
TXN TRANSFER A1 A2 400
PROCESS – process the next transaction in the queue.
ROLLBACK <n> – undo the last n successful transactions.
BALANCE <account> – show current balance for that account.
AUDIT – print the audit log (with rolled-back markers).
Usually there will also be an EXIT or you can end with EOF (Ctrl+D on Linux/macOS, Ctrl+Z then Enter on Windows).