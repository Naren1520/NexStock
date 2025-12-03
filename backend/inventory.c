#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX 100

struct Product {
    int id;
    char name[50];
    float price;
    int quantity;
};

struct Rental {
    long rentalId;
    int productId;
    char productName[50];
    char renterName[50];
    char rentDate[20];
    char returnDate[20];
    char phoneNumber[15];
    char address[100];
    float amountPaid;
    char status[20];
};

struct Product inventory[MAX];
struct Rental rentals[MAX];
int count = 0;
int rentalCount = 0;

void addProduct();
void displayProducts();
void searchProduct();
void sortByID();
void sortByName();
void sortByPrice();
void updateProduct();
void deleteProduct();
void sellProduct();
void recordRental();
void viewRentals();
void markReturnedRental();
void saveToFile();
void loadFromFile();
void saveToJSON();
void loadRentalsFromJSON();

int main() {
    int choice;
    loadFromFile();
    loadRentalsFromJSON();

    while (1) {
        printf("\n=== PRODUCT INVENTORY SYSTEM ===\n");
        printf("1. Add Product\n");
        printf("2. Display All Products\n");
        printf("3. Search Product by ID\n");
        printf("4. Sort Products by ID\n");
        printf("5. Sort Products by Name\n");
        printf("6. Sort Products by Price\n");
        printf("7. Update Product\n");
        printf("8. Delete Product\n");
        printf("9. Sell Product\n");
        printf("10. Record Rental\n");
        printf("11. View All Rentals\n");
        printf("12. Mark Rental as Returned\n");
        printf("13. Save & Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch(choice) {
            case 1: addProduct(); break;
            case 2: displayProducts(); break;
            case 3: searchProduct(); break;
            case 4: sortByID(); break;
            case 5: sortByName(); break;
            case 6: sortByPrice(); break;
            case 7: updateProduct(); break;
            case 8: deleteProduct(); break;
            case 9: sellProduct(); break;
            case 10: recordRental(); break;
            case 11: viewRentals(); break;
            case 12: markReturnedRental(); break;
            case 13: 
                saveToFile();
                printf("Data saved. Exiting...\n");
                return 0;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}

void addProduct() {
    if (count >= MAX) {
        printf("Inventory full!\n");
        return;
    }

    printf("\nEnter Product ID: ");
    scanf("%d", &inventory[count].id);

    // Check if ID already exists
    for (int i = 0; i < count; i++) {
        if (inventory[i].id == inventory[count].id) {
            printf("Product ID already exists!\n");
            return;
        }
    }

    printf("Enter Product Name: ");
    scanf("%s", inventory[count].name);
    printf("Enter Price: ");
    scanf("%f", &inventory[count].price);
    printf("Enter Quantity: ");
    scanf("%d", &inventory[count].quantity);

    count++;
    printf("Product added successfully.\n");
}

void displayProducts() {
    if (count == 0) {
        printf("\nNo products in inventory.\n");
        return;
    }

    printf("\n===== ALL PRODUCTS =====\n");
    printf("%-5s %-20s %-10s %-10s %-15s\n", "ID", "Name", "Price", "Qty", "Total Value");
    printf("====================================================\n");

    for (int i = 0; i < count; i++) {
        float totalValue = inventory[i].price * inventory[i].quantity;
        printf("%-5d %-20s %-10.2f %-10d %-15.2f\n",
               inventory[i].id,
               inventory[i].name,
               inventory[i].price,
               inventory[i].quantity,
               totalValue);
    }
}

void searchProduct() {
    int id, found = 0;
    printf("\nEnter product ID to search: ");
    scanf("%d", &id);

    for (int i = 0; i < count; i++) {
        if (inventory[i].id == id) {
            printf("\nProduct Found:\n");
            printf("ID: %d | Name: %s | Price: %.2f | Qty: %d\n",
                   inventory[i].id, inventory[i].name,
                   inventory[i].price, inventory[i].quantity);
            found = 1;
            break;
        }
    }

    if (!found)
        printf("Product not found.\n");
}

void sortByID() {
    struct Product temp;
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (inventory[i].id > inventory[j].id) {
                temp = inventory[i];
                inventory[i] = inventory[j];
                inventory[j] = temp;
            }
    printf("Sorted by ID.\n");
}

void sortByName() {
    struct Product temp;
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (strcmp(inventory[i].name, inventory[j].name) > 0) {
                temp = inventory[i];
                inventory[i] = inventory[j];
                inventory[j] = temp;
            }
    printf("Sorted by Name.\n");
}

void sortByPrice() {
    struct Product temp;
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (inventory[i].price > inventory[j].price) {
                temp = inventory[i];
                inventory[i] = inventory[j];
                inventory[j] = temp;
            }
    printf("Sorted by Price.\n");
}

void updateProduct() {
    int id, found = 0;
    printf("\nEnter product ID to update: ");
    scanf("%d", &id);

    for (int i = 0; i < count; i++) {
        if (inventory[i].id == id) {
            printf("Enter new name: ");
            scanf("%s", inventory[i].name);
            printf("Enter new price: ");
            scanf("%f", &inventory[i].price);
            printf("Enter new quantity: ");
            scanf("%d", &inventory[i].quantity);
            found = 1;
            printf("Product updated.\n");
            break;
        }
    }

    if (!found)
        printf("Product not found.\n");
}

void deleteProduct() {
    int id, found = 0;
    printf("\nEnter product ID to delete: ");
    scanf("%d", &id);

    for (int i = 0; i < count; i++) {
        if (inventory[i].id == id) {
            for (int j = i; j < count - 1; j++)
                inventory[j] = inventory[j + 1];
            count--;
            found = 1;
            printf("Product deleted.\n");
            break;
        }
    }

    if (!found)
        printf("Product not found.\n");
}

void sellProduct() {
    int id, qty, found = 0;
    printf("\nEnter product ID to sell: ");
    scanf("%d", &id);
    printf("Enter quantity to sell: ");
    scanf("%d", &qty);

    for (int i = 0; i < count; i++) {
        if (inventory[i].id == id) {
            if (inventory[i].quantity >= qty) {
                inventory[i].quantity -= qty;
                printf(" Sold %d unit(s) of %s\n", qty, inventory[i].name);
                printf("Remaining quantity: %d\n", inventory[i].quantity);
                found = 1;
            } else {
                printf("Insufficient quantity. Available: %d\n", inventory[i].quantity);
            }
            break;
        }
    }

    if (!found)
        printf("Product not found.\n");
}

void recordRental() {
    int id, found = 0;
    
    if (rentalCount >= MAX) {
        printf("Rental records full!\n");
        return;
    }

    printf("\nEnter product ID to rent: ");
    scanf("%d", &id);

    for (int i = 0; i < count; i++) {
        if (inventory[i].id == id) {
            printf("Enter renter name: ");
            scanf("%s", rentals[rentalCount].renterName);
            printf("Enter phone number: ");
            scanf("%s", rentals[rentalCount].phoneNumber);
            printf("Enter address: ");
            scanf("%s", rentals[rentalCount].address);
            printf("Enter return date (YYYY-MM-DD): ");
            scanf("%s", rentals[rentalCount].returnDate);
            printf("Enter amount paid: ");
            scanf("%f", &rentals[rentalCount].amountPaid);

            rentals[rentalCount].rentalId = (long)time(NULL);
            rentals[rentalCount].productId = id;
            strcpy(rentals[rentalCount].productName, inventory[i].name);
            strcpy(rentals[rentalCount].rentDate, __DATE__);
            strcpy(rentals[rentalCount].status, "active");

            printf("Rental recorded!\n");
            printf("Rental ID: %ld\n", rentals[rentalCount].rentalId);
            rentalCount++;
            found = 1;
            break;
        }
    }

    if (!found)
        printf("Product not found.\n");
}

void viewRentals() {
    if (rentalCount == 0) {
        printf("\nNo rental records.\n");
        return;
    }

    printf("\n===== RENTAL RECORDS =====\n");
    for (int i = 0; i < rentalCount; i++) {
        printf("\n--- Rental #%d ---\n", i + 1);
        printf("Rental ID: %ld\n", rentals[i].rentalId);
        printf("Product: %s (ID: %d)\n", rentals[i].productName, rentals[i].productId);
        printf("Renter: %s\n", rentals[i].renterName);
        printf("Phone: %s\n", rentals[i].phoneNumber);
        printf("Address: %s\n", rentals[i].address);
        printf("Rent Date: %s\n", rentals[i].rentDate);
        printf("Return Date: %s\n", rentals[i].returnDate);
        printf("Amount: ₹%.2f\n", rentals[i].amountPaid);
        printf("Status: %s\n", rentals[i].status);
    }
}

void markReturnedRental() {
    long id;
    int found = 0;

    printf("\nEnter rental ID to mark as returned: ");
    scanf("%ld", &id);

    for (int i = 0; i < rentalCount; i++) {
        if (rentals[i].rentalId == id) {
            if (strcmp(rentals[i].status, "active") == 0) {
                strcpy(rentals[i].status, "returned");
                printf("Rental marked as returned.\n");
            } else {
                printf("Rental is already marked as %s\n", rentals[i].status);
            }
            found = 1;
            break;
        }
    }

    if (!found)
        printf("Rental not found.\n");
}

void saveToFile() {
    FILE *fp = fopen("inventory.txt", "w");

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d %s %.2f %d\n",
                inventory[i].id,
                inventory[i].name,
                inventory[i].price,
                inventory[i].quantity);
    }
    fclose(fp);

    saveToJSON();
}

void loadFromFile() {
    FILE *fp = fopen("inventory.txt", "r");
    if (!fp) return;

    while (fscanf(fp, "%d %s %f %d",
                  &inventory[count].id,
                  inventory[count].name,
                  &inventory[count].price,
                  &inventory[count].quantity) != EOF) {
        count++;
    }

    fclose(fp);
}

void saveToJSON() {
    FILE *fp = fopen("inventory.json", "w");
    fprintf(fp, "{\n  \"products\": [\n");

    for (int i = 0; i < count; i++) {
        fprintf(fp,
            "    {\"id\": %d, \"name\": \"%s\", \"price\": %.2f, \"quantity\": %d}%s\n",
            inventory[i].id,
            inventory[i].name,
            inventory[i].price,
            inventory[i].quantity,
            (i == count - 1) ? "" : ","
        );
    }

    fprintf(fp, "  ],\n  \"rentals\": [\n");

    for (int i = 0; i < rentalCount; i++) {
        fprintf(fp,
            "    {\"rentalId\": %ld, \"productId\": %d, \"productName\": \"%s\", \"renterName\": \"%s\", \"rentDate\": \"%s\", \"returnDate\": \"%s\", \"phoneNumber\": \"%s\", \"address\": \"%s\", \"amountPaid\": %.2f, \"status\": \"%s\"}%s\n",
            rentals[i].rentalId,
            rentals[i].productId,
            rentals[i].productName,
            rentals[i].renterName,
            rentals[i].rentDate,
            rentals[i].returnDate,
            rentals[i].phoneNumber,
            rentals[i].address,
            rentals[i].amountPaid,
            rentals[i].status,
            (i == rentalCount - 1) ? "" : ","
        );
    }

    fprintf(fp, "  ]\n}\n");
    fclose(fp);

    printf("Data saved to inventory.json\n");
}

void loadRentalsFromJSON() {
    // For now, rentals are managed through the API
    // This is for future enhancements
}
