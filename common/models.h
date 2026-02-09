#ifndef MODELS_H
#define MODELS_H

typedef struct {
    int user_id;
    char name[50];
    char phone[15];
    char password[20];
} User;

typedef struct {
    int restaurant_id;
    char name[50];
} Restaurant;

typedef struct {
    int item_id;
    int restaurant_id;
    char name[50];
    float price;
} MenuItem;

typedef struct {
    int order_id;
    int user_id;
    int restaurant_id;
    int item_id;
    int quantity;
    float total_price;
    int status; // 0=PLACED,1=PREPARING,2=DELIVERED
} Order;

#endif

