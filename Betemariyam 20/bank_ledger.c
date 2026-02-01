#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCOUNTS 100
#define MAX_ID_LEN 32
#define MAX_TYPE_LEN 16
#define MAX_CMD_LEN 256

// -------------------- DATA STRUCTURES --------------------

// Account structure
typedef struct {
    char id[MAX_ID_LEN];
    long long balance;
    int in_use; // 0 = empty, 1 = used
} Account;

// Transaction types
typedef enum {
    TXN_DEPOSIT,
    TXN_WITHDRAW,
    TXN_TRANSFER
} TxnType;

// Transaction structure (for queue and stack)
typedef struct Transaction {
    int txn_id;
    TxnType type;
    char from_id[MAX_ID_LEN]; // for DEPOSIT/WITHDRAW this is the account
    char to_id[MAX_ID_LEN];   // for TRANSFER only
    long long amount;
} Transaction;

// Queue node for incoming transactions
typedef struct QueueNode {
    Transaction txn;
    struct QueueNode *next;
} QueueNode;

// Queue structure
typedef struct {
    QueueNode *front;
    QueueNode *rear;
} Queue;

// Stack node for successful transactions (for rollback)
typedef struct StackNode {
    Transaction txn;
    struct StackNode *next;
} StackNode;

// Audit log entry status
typedef enum {
    AUDIT_SUCCESS,
    AUDIT_FAIL,
    AUDIT_ROLLED_BACK
} AuditStatus;

// Audit log node (linked list)
typedef struct AuditNode {
    int index;              // 1-based position in log
    Transaction txn;
    AuditStatus status;
    char message[128];      // e.g., reason for failure or rollback note
    struct AuditNode *next;
} AuditNode;

// -------------------- GLOBALS --------------------

Account accounts[MAX_ACCOUNTS];
int account_count = 0;

Queue txn_queue = { NULL, NULL };
StackNode *success_stack = NULL;

AuditNode *audit_head = NULL;
AuditNode *audit_tail = NULL;
int audit_count = 0;

int next_txn_id = 1;

// -------------------- UTILITY FUNCTIONS --------------------

// Find account by id, return pointer or NULL
Account* find_account(const char *id) {
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        if (accounts[i].in_use && strcmp(accounts[i].id, id) == 0) {
            return &accounts[i];
        }
    }
    return NULL;
}

// Create account
void cmd_create_account(const char *id, long long initial_balance) {
    if (find_account(id) != NULL) {
        printf("Error: Account %s already exists.\n", id);
        return;
    }
    // Find free slot
    int idx = -1;
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        if (!accounts[i].in_use) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        printf("Error: Maximum number of accounts reached.\n");
        return;
    }
    strncpy(accounts[idx].id, id, MAX_ID_LEN - 1);
    accounts[idx].id[MAX_ID_LEN - 1] = '\0';
    accounts[idx].balance = initial_balance;
    accounts[idx].in_use = 1;
    account_count++;
    printf("Created account %s with balance %lld.\n", id, initial_balance);
}

// -------------------- QUEUE FUNCTIONS --------------------

void enqueue(Transaction txn) {
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    node->txn = txn;
    node->next = NULL;
    if (txn_queue.rear == NULL) {
        txn_queue.front = txn_queue.rear = node;
    } else {
        txn_queue.rear->next = node;
        txn_queue.rear = node;
    }
}

int is_queue_empty() {
    return txn_queue.front == NULL;
}

Transaction dequeue() {
    QueueNode *node = txn_queue.front;
    Transaction t = node->txn;
    txn_queue.front = node->next;
    if (txn_queue.front == NULL) {
        txn_queue.rear = NULL;
    }
    free(node);
    return t;
}

// -------------------- STACK FUNCTIONS --------------------

void push_success(Transaction txn) {
    StackNode *node = (StackNode *)malloc(sizeof(StackNode));
    node->txn = txn;
    node->next = success_stack;
    success_stack = node;
}

int is_stack_empty() {
    return success_stack == NULL;
}

Transaction pop_success() {
    StackNode *node = success_stack;
    Transaction t = node->txn;
    success_stack = node->next;
    free(node);
    return t;
}

// -------------------- AUDIT LOG (LINKED LIST) --------------------

void append_audit(Transaction txn, AuditStatus status, const char *message) {
    AuditNode *node = (AuditNode *)malloc(sizeof(AuditNode));
    node->txn = txn;
    node->status = status;
    node->next = NULL;
    node->index = ++audit_count;
    strncpy(node->message, message, sizeof(node->message) - 1);
    node->message[sizeof(node->message) - 1] = '\0';

    if (audit_tail == NULL) {
        audit_head = audit_tail = node;
    } else {
        audit_tail->next = node;
        audit_tail = node;
    }
}

// After rollback, mark in audit log
void mark_rolled_back(int txn_id) {
    AuditNode *cur = audit_head;
    while (cur != NULL) {
        if (cur->txn.txn_id == txn_id && cur->status == AUDIT_SUCCESS) {
            cur->status = AUDIT_ROLLED_BACK;
            snprintf(cur->message, sizeof(cur->message),
                     "Rolled back transaction %d", txn_id);
            return;
        }
        cur = cur->next;
    }
}

// Print full audit log
void cmd_audit() {
    AuditNode *cur = audit_head;
    while (cur != NULL) {
        const char *type_str = "";
        if (cur->txn.type == TXN_DEPOSIT) type_str = "DEPOSIT";
        else if (cur->txn.type == TXN_WITHDRAW) type_str = "WITHDRAW";
        else if (cur->txn.type == TXN_TRANSFER) type_str = "TRANSFER";

        const char *status_str = "";
        if (cur->status == AUDIT_SUCCESS) status_str = "SUCCESS";
        else if (cur->status == AUDIT_FAIL) status_str = "FAIL";
        else if (cur->status == AUDIT_ROLLED_BACK) status_str = "ROLLED_BACK";

        printf("%d. %s ", cur->index, type_str);
        if (cur->txn.type == TXN_TRANSFER) {
            printf("%s->%s %lld - %s",
                   cur->txn.from_id, cur->txn.to_id,
                   cur->txn.amount, status_str);
        } else {
            printf("%s %lld - %s",
                   cur->txn.from_id,
                   cur->txn.amount,
                   status_str);
        }
        if (strlen(cur->message) > 0) {
            printf(" (%s)", cur->message);
        }
        printf("\n");
        cur = cur->next;
    }
}

// -------------------- PROCESSING LOGIC --------------------

// Parse transaction type string
int parse_type(const char *type_str, TxnType *out_type) {
    if (strcmp(type_str, "DEPOSIT") == 0) {
        *out_type = TXN_DEPOSIT;
        return 1;
    } else if (strcmp(type_str, "WITHDRAW") == 0) {
        *out_type = TXN_WITHDRAW;
        return 1;
    } else if (strcmp(type_str, "TRANSFER") == 0) {
        *out_type = TXN_TRANSFER;
        return 1;
    }
    return 0;
}

// TXN command: queue transaction
void cmd_txn(char **tokens, int count) {
    if (count < 4) {
        printf("Error: Invalid TXN command.\n");
        return;
    }

    TxnType type;
    if (!parse_type(tokens[1], &type)) {
        printf("Error: Invalid transaction type.\n");
        return;
    }

    Transaction txn;
    txn.txn_id = next_txn_id++;
    txn.type = type;
    txn.amount = 0;
    txn.from_id[0] = '\0';
    txn.to_id[0] = '\0';

    if (type == TXN_DEPOSIT || type == TXN_WITHDRAW) {
        if (count != 4) {
            printf("Error: Invalid TXN command format for DEPOSIT/WITHDRAW.\n");
            return;
        }
        strncpy(txn.from_id, tokens[2], MAX_ID_LEN - 1);
        txn.from_id[MAX_ID_LEN - 1] = '\0';
        txn.amount = atoll(tokens[3]);
    } else if (type == TXN_TRANSFER) {
        if (count != 5) {
            printf("Error: Invalid TXN command format for TRANSFER.\n");
            return;
        }
        strncpy(txn.from_id, tokens[2], MAX_ID_LEN - 1);
        txn.from_id[MAX_ID_LEN - 1] = '\0';
        strncpy(txn.to_id, tokens[3], MAX_ID_LEN - 1);
        txn.to_id[MAX_ID_LEN - 1] = '\0';
        txn.amount = atoll(tokens[4]);
    }

    enqueue(txn);
    printf("Queued transaction %d.\n", txn.txn_id);
}

// PROCESS command: process next transaction in FIFO order
void cmd_process() {
    if (is_queue_empty()) {
        printf("No transactions to process.\n");
        return;
    }

    Transaction txn = dequeue();

    Account *from = NULL;
    Account *to = NULL;
    int success = 0;
    char msg[128] = "";

    if (txn.type == TXN_DEPOSIT) {
        from = find_account(txn.from_id);
        if (!from) {
            snprintf(msg, sizeof(msg), "Account %s not found", txn.from_id);
            append_audit(txn, AUDIT_FAIL, msg);
            printf("Failure: DEPOSIT %s %lld. %s.\n",
                   txn.from_id, txn.amount, msg);
            return;
        }
        from->balance += txn.amount;
        success = 1;
        snprintf(msg, sizeof(msg),
                 "Balance: %lld", from->balance);
        printf("Success: DEPOSIT %s %lld. Balance: %lld.\n",
               txn.from_id, txn.amount, from->balance);
    } else if (txn.type == TXN_WITHDRAW) {
        from = find_account(txn.from_id);
        if (!from) {
            snprintf(msg, sizeof(msg), "Account %s not found", txn.from_id);
            append_audit(txn, AUDIT_FAIL, msg);
            printf("Failure: WITHDRAW %s %lld. %s.\n",
                   txn.from_id, txn.amount, msg);
            return;
        }
        if (from->balance < txn.amount) {
            snprintf(msg, sizeof(msg), "Insufficient funds");
            append_audit(txn, AUDIT_FAIL, msg);
            printf("Failure: WITHDRAW %s %lld. %s.\n",
                   txn.from_id, txn.amount, msg);
            return;
        }
        from->balance -= txn.amount;
        success = 1;
        snprintf(msg, sizeof(msg),
                 "Balance: %lld", from->balance);
        printf("Success: WITHDRAW %s %lld. Balance: %lld.\n",
               txn.from_id, txn.amount, from->balance);
    } else if (txn.type == TXN_TRANSFER) {
        from = find_account(txn.from_id);
        to = find_account(txn.to_id);
        if (!from || !to) {
            snprintf(msg, sizeof(msg), "Account not found");
            append_audit(txn, AUDIT_FAIL, msg);
            printf("Failure: TRANSFER %s->%s %lld. %s.\n",
                   txn.from_id, txn.to_id, txn.amount, msg);
            return;
        }
        if (from->balance < txn.amount) {
            snprintf(msg, sizeof(msg), "Insufficient funds");
            append_audit(txn, AUDIT_FAIL, msg);
            printf("Failure: TRANSFER %s->%s %lld. %s.\n",
                   txn.from_id, txn.to_id, txn.amount, msg);
            return;
        }
        from->balance -= txn.amount;
        to->balance += txn.amount;
        success = 1;
        snprintf(msg, sizeof(msg),
                 "A1: %lld, A2: %lld",
                 from->balance, to->balance);
        printf("Success: TRANSFER %s->%s %lld. %s: %lld, %s: %lld.\n",
               txn.from_id, txn.to_id, txn.amount,
               txn.from_id, from->balance,
               txn.to_id, to->balance);
    }

    if (success) {
        append_audit(txn, AUDIT_SUCCESS, msg);
        push_success(txn);
    }
}

// ROLLBACK command: reverse last n successful transactions
void cmd_rollback(int n) {
    if (n <= 0) {
        printf("Error: ROLLBACK n must be positive.\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (is_stack_empty()) {
            printf("No more transactions to rollback.\n");
            return;
        }
        Transaction txn = pop_success();

        Account *from = NULL;
        Account *to = NULL;

        if (txn.type == TXN_DEPOSIT) {
            from = find_account(txn.from_id);
            if (from) {
                from->balance -= txn.amount;
                printf("Rolled back: DEPOSIT %s %lld. %s: %lld.\n",
                       txn.from_id, txn.amount,
                       txn.from_id, from->balance);
            }
        } else if (txn.type == TXN_WITHDRAW) {
            from = find_account(txn.from_id);
            if (from) {
                from->balance += txn.amount;
                printf("Rolled back: WITHDRAW %s %lld. %s: %lld.\n",
                       txn.from_id, txn.amount,
                       txn.from_id, from->balance);
            }
        } else if (txn.type == TXN_TRANSFER) {
            from = find_account(txn.from_id);
            to = find_account(txn.to_id);
            if (from && to) {
                // reverse transfer: give back to source
                from->balance += txn.amount;
                to->balance -= txn.amount;
                printf("Rolled back: TRANSFER %s->%s %lld. %s: %lld, %s: %lld.\n",
                       txn.from_id, txn.to_id, txn.amount,
                       txn.from_id, from->balance,
                       txn.to_id, to->balance);
            }
        }
        // Mark in audit log
        mark_rolled_back(txn.txn_id);
    }
}

// BALANCE command
void cmd_balance(const char *id) {
    Account *acc = find_account(id);
    if (!acc) {
        printf("Error: Account %s not found.\n", id);
        return;
    }
    printf("%s: %lld\n", id, acc->balance);
}

// -------------------- COMMAND PARSING --------------------

// Simple tokenizer: split line into tokens by space
int tokenize(char *line, char **tokens, int max_tokens) {
    int count = 0;
    char *p = strtok(line, " \t\r\n");
    while (p != NULL && count < max_tokens) {
        tokens[count++] = p;
        p = strtok(NULL, " \t\r\n");
    }
    return count;
}

// Main loop
int main() {
    char line[MAX_CMD_LEN];

    printf("Bank Transaction Ledger. Enter commands (CTRL+D to exit).\n");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        // Ignore empty lines
        if (line[0] == '\n' || line[0] == '\r') continue;

        char *tokens[8];
        int count = tokenize(line, tokens, 8);
        if (count == 0) continue;

        if (strcmp(tokens[0], "CREATE_ACCOUNT") == 0) {
            if (count != 3) {
                printf("Usage: CREATE_ACCOUNT <id> <initial_balance>\n");
                continue;
            }
            char *id = tokens[1];
            long long bal = atoll(tokens[2]);
            cmd_create_account(id, bal);
        } else if (strcmp(tokens[0], "TXN") == 0) {
            cmd_txn(tokens, count);
        } else if (strcmp(tokens[0], "PROCESS") == 0) {
            cmd_process();
        } else if (strcmp(tokens[0], "ROLLBACK") == 0) {
            if (count != 2) {
                printf("Usage: ROLLBACK <n>\n");
                continue;
            }
            int n = atoi(tokens[1]);
            cmd_rollback(n);
        } else if (strcmp(tokens[0], "BALANCE") == 0) {
            if (count != 2) {
                printf("Usage: BALANCE <account>\n");
                continue;
            }
            cmd_balance(tokens[1]);
        } else if (strcmp(tokens[0], "AUDIT") == 0) {
            cmd_audit();
        } else {
            printf("Unknown command: %s\n", tokens[0]);
        }
    }

    return 0;
}
