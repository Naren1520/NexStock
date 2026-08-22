# NexStock - Inventory Management System
## A C + Web Application

---

## Table of Contents
NexStock is an inventory and rental management application with a C HTTP backend and a browser frontend. The backend serves the files in `frontend/`, implements the REST API in `backend/inventory-api.c`, and persists data in MongoDB. There is no Node.js backend.
6. [Complete User Journey](#complete-user-journey)
7. [API Endpoints](#api-endpoints)
## Configuration

Copy `.env.example` to `.env` and set:

```text
MONGODB_URI=mongodb+srv://username:password@cluster.example.mongodb.net/?retryWrites=true&w=majority
MONGODB_DATABASE=nexstock
GEMINI_API_KEY=your_gemini_key
PORT=8040
```

`MONGODB_DATABASE` defaults to `nexstock`; Render supplies `PORT` automatically. The C process checks MongoDB connectivity at startup and exits if `MONGODB_URI` is missing or unreachable.

**NexStock** is an advanced inventory management system that demonstrates the power of combining:
Docker is the recommended local build because it supplies libmicrohttpd, cJSON, libmongoc, libbson, and OpenSSL:

```bash
docker build -t nexstock-c-backend .
docker run --rm --env-file .env -p 8040:8040 nexstock-c-backend
```

Open `http://localhost:8040/login.html`. Render uses `Dockerfile` and `render.yaml`; configure the secret `MONGODB_URI` and `GEMINI_API_KEY` in the Render dashboard.
- **C HTTP Server** - REST API handling and inventory logic
- **JavaScript (Frontend)** - User interface
`POST /api/auth/signup` and `POST /api/auth/login` accept `{ "email": "...", "password": "..." }` and return `success`, `token`, and `user`. The frontend stores the token in `localStorage` and sends it as `Authorization: Bearer <token>`.

All `/api/c/*` inventory and rental routes require a valid bearer token. `/api/auth/*` and `GET /api/chatbot-key` are public. Sessions live only in memory and are invalidated when the backend restarts.

The project is built to show how **pure C algorithms** can be integrated with modern web technologies.
Existing response contracts are preserved for:

- `GET /api/c/products` and `GET /api/c/rentals`
- `POST /api/c/product/add`
- `PUT /api/c/product/update`
- `DELETE /api/c/product/delete`
- `POST /api/c/product/sell`
- `GET /api/c/product/search/:id`
- `GET /api/c/product/sort/:field`
- `POST /api/c/rental/record`
- `PUT /api/c/rental/return`
│  (HTML/CSS/JavaScript Frontend Interface)                   │
│                                                             │
```text
backend/inventory-api.c   C HTTP server, auth, MongoDB persistence
backend/inventory.c       C data-structure reference implementation
backend/inventory.json     Legacy seed/reference data; API persistence is MongoDB
frontend/auth.js           Session guard and authenticated fetch wrapper
frontend/login.html        Signup and login UI
frontend/*.html            Browser application pages
Dockerfile                 Multi-stage C build and runtime image
render.yaml                Render service and environment configuration
```
│                                                             │
│  - Handles HTTP requests                                    │
Check container logs first. Confirm that the MongoDB deployment permits the service network, the URI credentials are valid, and the database user can read and write the three collections. A missing or unreachable MongoDB URI prevents startup by design.
│  - Returns JSON responses                                   │
│  - Serves static files (HTML/CSS/JS)                        │
└──────────────────────┬──────────────────────────────────────┘
                       │ Function Calls
                       ↓
┌─────────────────────────────────────────────────────────────┐
│                C BACKEND (inventory-api.c)                   │
│  (C server implementing inventory logic)                    │
│                                                             │
│  - addProduct()        - sellProduct()                      │
│  - deleteProduct()     - recordRental()                     │
│  - updateProduct()     - markRentalReturned()               │
│  - searchProduct()     - getAllRentals()                    │
│  - sortByID()          - readInventory()                    │
│  - sortByName()        - writeInventory()                   │
│  - sortByPrice()                                            │
└──────────────────────┬──────────────────────────────────────┘
                       │ File Read/Write
                       ↓
┌──────────────────────────────────────────────────────────────┐
│              INVENTORY.JSON (Data Storage)                   │
│                                                              │
│  {                                                           │
│    "products": [                                             │
│      {id, name, price, quantity},                            │
│      ...                                                     │
│    ],                                                        │
│    "rentals": [                                              │
│      {rentalId, productId, renterName, ...},                 │
│      ...                                                     │
│    ]                                                         │
│  }                                                           │
└──────────────────────────────────────────────────────────────┘
```

---

## C Data Structures

### 1. **Product Structure** (inventory.c)

```c
struct Product {
    int id;              // Unique product identifier
    char name[50];       // Product name (max 50 chars)
    float price;         // Product price in currency
    int quantity;        // Current stock quantity
};

struct Product inventory[MAX];  // Array of 100 products max
int count = 0;                  // Current number of products
```

**Example in Memory:**
```
inventory[0] = {id: 1, name: "Laptop", price: 50000.00, quantity: 5}
inventory[1] = {id: 2, name: "Mouse", price: 500.00, quantity: 25}
inventory[2] = {id: 3, name: "Keyboard", price: 1200.00, quantity: 15}
```

### 2. **Rental Structure** (inventory.c)

```c
struct Rental {
    long rentalId;           // Unique rental ID (timestamp)
    int productId;           // Which product is rented
    char productName[50];    // Product name (for quick access)
    char renterName[50];     // Who rented it
    char rentDate[20];       // When it was rented (YYYY-MM-DD)
    char returnDate[20];     // Expected return date
    char phoneNumber[15];    // Renter's phone
    char address[100];       // Renter's address
    float amountPaid;        // Rental fee paid
    char status[20];         // "active" or "returned"
};

struct Rental rentals[MAX];  // Array of 100 rentals max
int rentalCount = 0;         // Current number of rentals
```

**Example in Memory:**
```
rentals[0] = {
    rentalId: 1702250400000,
    productId: 1,
    productName: "Laptop",
    renterName: "Naren",
    rentDate: "2025-12-10",
    returnDate: "2025-12-17",
    phoneNumber: "9876543210",
    address: "123 Main St, City",
    amountPaid: 5000.00,
    status: "active"
}
```

---

## How the System Works

### **Complete Data Flow Process**

#### Step 1: **User Interaction (Frontend)**
```
User opens browser → http://localhost:8080
         ↓
Page loads (HTML/CSS/JS from server)
         ↓
JavaScript loadData() function executes
         ↓
fetch('/api/c/products') - HTTP GET request
```

#### Step 2: **Server Processing (C HTTP Server)**
```
C server receives: GET /api/c/products
         ↓
Matches route: if (pathname === '/api/c/products')
         ↓
Calls: cBackend.readInventory()
         ↓
Returns: Array of all products
```

#### Step 3: **C Backend Logic (inventory-api.c)**
```
readInventory() function:
         ↓
Reads file: backend/inventory.json
         ↓
Parses JSON to JavaScript objects
         ↓
Returns: { products: [...], rentals: [...] }
```

#### Step 4: **Response Back to Browser**
```
Server sends JSON response (200 OK)
         ↓
Frontend JavaScript receives data
         ↓
displayProducts() function creates HTML
         ↓
updateCharts() renders charts
         ↓
User sees dashboard with data
```

---

## Frontend to Backend Flow

### **Example 1: Adding a Product**

#### Frontend Code (products.html)
```javascript
async function addProduct(e) {
    e.preventDefault();
    const id = parseInt(document.getElementById("productId").value);
    const name = document.getElementById("productName").value.trim();
    const price = parseFloat(document.getElementById("productPrice").value);
    const quantity = parseInt(document.getElementById("productQuantity").value);

    // Step 1: User clicks "Add Product" button
    // Step 2: JavaScript sends HTTP POST request
    const res = await fetch("/api/c/product/add", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ id, name, price, quantity })
    });
}
```

#### C server receives POST /api/c/product/add
```javascript
if (pathname === '/api/c/product/add' && req.method === 'POST') {
    parseBody(req, (err, data) => {
        const { id, name, price, quantity } = data;
        
        // Step 3: Call C backend function
        const result = cBackend.addProduct(id, name, price, quantity);
        
        // Step 4: Send result back
        if (result.success) {
            res.writeHead(201);
            res.end(JSON.stringify(result));
        }
    });
}
```

#### C Backend Function (inventory-api.c)
```javascript
function addProduct(id, name, price, quantity) {
    // Step 5: Read current inventory
    const inventory = readInventory();
    
    // Step 6: Check if ID already exists (validation)
    if (inventory.products.find(p => p.id === parseInt(id))) {
        return { success: false, error: 'Product ID already exists' };
    }

    // Step 7: Add new product to array
    inventory.products.push({
        id: parseInt(id),
        name: name.trim(),
        price: parseFloat(price),
        quantity: parseInt(quantity)
    });

    // Step 8: Write back to MongoDB
    if (writeInventory(inventory)) {
        return { success: true, message: 'Product added' };
    }
}
```

#### Data Persistence (inventory.json)
```json
{
  "products": [
    {"id": 1, "name": "Laptop", "price": 50000.00, "quantity": 5},
    {"id": 2, "name": "Mouse", "price": 500.00, "quantity": 25},
    {"id": 3, "name": "New Product", "price": 1000.00, "quantity": 10}
  ],
  "rentals": []
}
```

#### Frontend receives response and updates (products.html)
```javascript
const result = await res.json();
if (result.success) {
    showSuccess("Product added successfully!");
    await loadData();  // Reload all products
    displayProducts(); // Update table on screen
    updateCharts();    // Update charts
}
```

---

### **Example 2: Sorting Products by Name**

#### User clicks "Sort by Name" button
```
Frontend JavaScript → sortByName()
         ↓
fetch('/api/c/product/sort/name')
         ↓
Server receives GET /api/c/product/sort/name
         ↓
Calls cBackend.sortByName()
```

#### C Backend Sorting Algorithm (inventory-api.c)
```javascript
function sortByName() {
    const inventory = readInventory();
    
    // Bubble Sort Algorithm (similar to C)
    inventory.products.sort((a, b) => a.name.localeCompare(b.name));
    
    writeInventory(inventory);
    return { success: true, message: 'Sorted by Name', products: inventory.products };
}
```

#### This is equivalent to C code:
```c
void sortByName() {
    struct Product temp;
    for (int i = 0; i < count - 1; i++)
        for (int j = i + 1; j < count; j++)
            if (strcmp(inventory[i].name, inventory[j].name) > 0) {
                temp = inventory[i];
                inventory[i] = inventory[j];
                inventory[j] = temp;
            }
}
```

---

### **Example 3: Recording a Rental**

#### User fills rental form and clicks "Record Rental"
```
Frontend: /api/c/rental/record (POST)
         ↓
Body: {
    productId: 1,
    renterName: "Naren",
    returnDate: "2025-12-17",
    phoneNumber: "9876543210",
    address: "123 Main St",
    amountPaid: 5000.00
}
```

#### C server processes rental
```javascript
if (pathname === '/api/c/rental/record' && req.method === 'POST') {
    parseBody(req, (err, data) => {
        const result = cBackend.recordRental(
            data.productId,
            data.renterName,
            data.returnDate,
            data.phoneNumber,
            data.address,
            data.amountPaid
        );
    });
}
```

#### C Backend records rental (inventory-api.c)
```javascript
function recordRental(productId, renterName, returnDate, phoneNumber, address, amountPaid) {
    const inventory = readInventory();
    const productIndex = inventory.products.findIndex(p => p.id === parseInt(productId));

    // Check if product exists
    if (productIndex === -1) {
        return { success: false, error: 'Product not found' };
    }

    // Check if product is available
    if (inventory.products[productIndex].quantity <= 0) {
        return { success: false, error: 'Product not available for rent' };
    }

    // Decrease quantity (product is now rented out)
    inventory.products[productIndex].quantity--;

    // Create rental record
    const rental = {
        rentalId: Date.now(),        // Unique ID
        productId: parseInt(productId),
        productName: inventory.products[productIndex].name,
        renterName: renterName.trim(),
        rentDate: new Date().toISOString().split('T')[0],
        returnDate: returnDate,
        phoneNumber: phoneNumber.trim(),
        address: address.trim(),
        amountPaid: parseFloat(amountPaid),
        status: 'active'
    };

    // Add rental to array
    inventory.rentals.push(rental);

    // Save to file
    if (writeInventory(inventory)) {
        return { success: true, message: 'Rental recorded', rental: rental };
    }
}
```

---

## Complete User Journey

### **Scenario: Product Lifecycle**

#### 1. **Add a Product**
```
User: Opens "Add Product" page
      ↓
      Enters: ID=1, Name="Laptop", Price=50000, Quantity=5
      ↓
      Click: "Add Product" button
      ↓
Frontend: Sends POST /api/c/product/add
      ↓
C Backend: 
  - Validates ID doesn't exist
  - Creates Product struct
  - Adds to inventory array
  - Writes to inventory.json
      ↓
Response: { success: true, message: 'Product added' }
      ↓
Frontend: Reloads page, displays "Laptop" in products table
```

#### 2. **View Product on Dashboard**
```
User: Clicks "Dashboard" tab
      ↓
Frontend: Calls fetch('/api/c/products')
      ↓
C Backend: 
  - Reads inventory.json
  - Returns all products array
      ↓
Frontend: 
  - Updates stats (Total Products, Total Value, etc.)
  - Creates charts from product data
  - Shows Laptop: ₹50000 * 5 = ₹250000 value
```

#### 3. **Sort Products by Price**
```
User: Clicks "Sort by Price"
      ↓
Frontend: Calls fetch('/api/c/product/sort/price')
      ↓
C Backend: 
  - Reads inventory.json
  - Sorts products array by price (Bubble Sort)
  - Writes back to inventory.json
  - Returns sorted array
      ↓
Frontend: Displays products in price order
```

#### 4. **Record Rental**
```
User: Goes to product, clicks "Rent"
      ↓
      Fills: Name, Phone, Address, Return Date, Amount
      ↓
      Clicks: "Record Rental"
      ↓
Frontend: Sends POST /api/c/rental/record
      ↓
C Backend: 
  - Finds product (Laptop)
  - Decreases quantity: 5 → 4
  - Creates Rental struct with all details
  - Adds rental to rentals array
  - Writes both changes to inventory.json
      ↓
Response: { success: true, rental: {...} }
      ↓
Frontend: Shows "Rental recorded! Rental ID: 1702250400000"
           Redirects to rentals page
```

#### 5. **View Rentals**
```
User: Goes to "Rentals" page
      ↓
Frontend: Calls fetch('/api/c/rentals')
      ↓
C Backend: 
  - Reads inventory.json
  - Returns rentals array
      ↓
Frontend: Shows rental table with status filters (Active/Returned)
```

#### 6. **Mark Rental as Returned**
```
User: Clicks "Return" button on rental record
      ↓
Frontend: Sends PUT /api/c/rental/return
      ↓
C Backend: 
  - Finds rental by rentalId
  - Changes status: "active" → "returned"
  - Finds original product (Laptop)
  - Increases quantity: 4 → 5
  - Writes changes to inventory.json
      ↓
Frontend: Updates rental status, redirects to rentals page
```

---

## 🔌 API Endpoints

### **Product Endpoints**

| Method | Endpoint | Purpose | C Function |
|--------|----------|---------|-----------|
| GET | `/api/c/products` | Get all products | readInventory() |
| POST | `/api/c/product/add` | Add new product | addProduct() |
| PUT | `/api/c/product/update` | Update product | updateProduct() |
| DELETE | `/api/c/product/delete` | Delete product | deleteProduct() |
| POST | `/api/c/product/sell` | Sell product | sellProduct() |
| GET | `/api/c/product/search/:id` | Search by ID | searchProduct() |
| GET | `/api/c/product/sort/id` | Sort by ID | sortByID() |
| GET | `/api/c/product/sort/name` | Sort by name | sortByName() |
| GET | `/api/c/product/sort/price` | Sort by price | sortByPrice() |

### **Rental Endpoints**

| Method | Endpoint | Purpose | C Function |
|--------|----------|---------|-----------|
| GET | `/api/c/rentals` | Get all rentals | getAllRentals() |
| POST | `/api/c/rental/record` | Record new rental | recordRental() |
| PUT | `/api/c/rental/return` | Mark returned | markRentalReturned() |

---

##  Data Storage Format

### **MongoDB Collection Structure**

```json
{
  "products": [
    {
      "id": 1,
      "name": "Laptop",
      "price": 50000.00,
      "quantity": 5
    },
    {
      "id": 2,
      "name": "Mouse",
      "price": 500.00,
      "quantity": 25
    }
  ],
  "rentals": [
    {
      "rentalId": 1702250400000,
      "productId": 1,
      "productName": "Laptop",
      "renterName": "Naren",
      "rentDate": "2025-12-10",
      "returnDate": "2025-12-17",
      "phoneNumber": "9876543210",
      "address": "123 Main St, City",
      "amountPaid": 5000.00,
      "status": "active"
    }
  ]
}
```

---

##  How Algorithms Work

### **1. Add Product Algorithm**
```
Input: id, name, price, quantity
Process:
  1. Read inventory from JSON
  2. Check if ID already exists
  3. If exists: return error
  4. If not: add to products array
  5. Write back to JSON
  6. Return success
Output: { success: true/false, ... }
Time Complexity: O(n) - linear search for duplicate check
```

### **2. Sort by Name Algorithm** (Bubble Sort)
```
Input: products array
Process:
  1. Read inventory from JSON
  2. Compare adjacent product names
  3. Swap if first > second (alphabetically)
  4. Repeat until sorted
  5. Write back to JSON
  6. Return sorted array
Output: sorted products array
Time Complexity: O(n²) - bubble sort
Space Complexity: O(1) - in-place sorting
```

### **3. Search Product Algorithm**
```
Input: product ID
Process:
  1. Read inventory from JSON
  2. Loop through products array
  3. Compare each product ID with search ID
  4. If found: return product
  5. If not found: return error
Output: product object or error
Time Complexity: O(n) - linear search
```

### **4. Record Rental Algorithm**
```
Input: productId, renterName, returnDate, ...
Process:
  1. Read inventory from JSON
  2. Find product by ID
  3. If product not found: return error
  4. If quantity <= 0: return error
  5. Decrease product quantity by 1
  6. Create rental struct with all fields
  7. Add rental to rentals array
  8. Write both changes to JSON
Output: { success: true, rental: {...} }
Time Complexity: O(n) - linear search for product
```

---

##  Installation & Setup

### **Prerequisites**
- Docker
- A MongoDB deployment (Atlas or self-hosted)
- GCC compiler (for C compilation)
- Git (optional)

### **Step 1: Extract Project**
```bash
cd DatastructureProject
```

### **Step 2: Build the C Backend**
```bash
docker build -t nexstock-c-backend .
```

---

## Running the Project

### **C Backend Deployment (Render)**
The deployed application runs from `backend/inventory-api.c`. It is a C HTTP server that serves the frontend, exposes the `/api/c/...` inventory and rental endpoints, and reads and writes MongoDB collections. JavaScript is used by the browser UI and chatbot only.

Render uses the repository `Dockerfile` and `render.yaml`; set `MONGODB_URI`, `MONGODB_DATABASE`, and `GEMINI_API_KEY` as Render environment variables. The service automatically uses Render's `PORT` value.

To run the same service locally with Docker:
```bash
docker build -t nexstock-c-backend .
docker run --rm --env-file .env -p 8040:8040 nexstock-c-backend
```
Then open `http://localhost:8040`.

Copy `.env.example` to `.env` and set `MONGODB_URI`, `MONGODB_DATABASE`, and `GEMINI_API_KEY`. Render environment variables are configured in the Render dashboard from the `render.yaml` definition.

### Authentication
Open `/login.html` to create an account or sign in. Passwords are stored as SHA-256 hashes in the `users` collection. The browser stores the returned in-memory session token in `localStorage`; every `/api/c/*` request must include it as a bearer token. Auth sessions are intentionally lost when the C process restarts.

Public endpoints are `POST /api/auth/signup`, `POST /api/auth/login`, and `GET /api/chatbot-key`. Products and rentals are stored in the `products` and `rentals` MongoDB collections.

### **Method 1: Web Interface (C Backend)**
```bash
docker build -t nexstock-c-backend .
docker run --rm -p 8040:8040 -e GEMINI_API_KEY=your_key nexstock-c-backend
```
Then open: `http://localhost:8040`

**Features available:**
- Dashboard with charts
- Product CRUD operations
- Rental management
- Search & filter
- Analytics

---

## Project Structure

```
DatastructureProject/
│
├── README.md                       ← This file
├── Dockerfile                      ← C backend container build
│
├── backend/
│   ├── inventory.c                 ← Pure C source code (algorithms)
│   ├── inventory-api.c             ← C HTTP backend
│   ├── inventory.json              ← Data storage (products + rentals)
│   ├── inventory.txt               ← Backup file
│   └── inventory.json.bak.*        ← Backups
│
└── frontend/
    ├── index.html                  ← Home page
    ├── dashboard.html              ← Dashboard with charts
    ├── products.html               ← Product management
    ├── add-product.html            ← Add product form
    ├── search.html                 ← Search functionality
    ├── analytics.html              ← Advanced analytics
    ├── rentals.html                ← Rental management
    ├── style.css                   ← Global styles
    ├── script.js                   ← Utility functions
    ├── profile-icon.js             ← Profile icon handler
    └── assests/                    ← Images & assets
```

---

## Data Flow Diagram - Click to Action

```
┌─────────────────────────────────────────────────────────────┐
│                                                              │
│  FRONTEND (User Action)                                     │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ User clicks "Add Product" button                       │ │
│  │ ↓                                                      │ │
│  │ JavaScript collects form data:                        │ │
│  │ {id: 1, name: "Laptop", price: 50000, qty: 5}        │ │
│  │ ↓                                                      │ │
│  │ fetch('/api/c/product/add', { POST, JSON body })     │ │
│  └────────────────────────────────────────────────────────┘ │
│                        ↓ HTTP Request
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  C SERVER (inventory-api.c)                                 │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Receives: POST /api/c/product/add                     │ │
│  │ ↓                                                      │ │
│  │ parseBody() extracts JSON data                        │ │
│  │ ↓                                                      │ │
│  │ Validates endpoint matches                           │ │
│  │ ↓                                                      │ │
│  │ Calls: cBackend.addProduct(1, "Laptop", ...)         │ │
│  │ ↓                                                      │ │
│  │ Receives result from C backend                       │ │
│  │ ↓                                                      │ │
│  │ res.writeHead(201) + res.end(JSON.stringify(result)) │ │
│  └────────────────────────────────────────────────────────┘ │
│                        ↓ HTTP Response (JSON)
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  C BACKEND (inventory-api.c)                                │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ addProduct(id, name, price, quantity)                │ │
│  │ ↓                                                      │ │
│  │ readInventory() {                                     │ │
│  │   read inventory.json file                           │ │
│  │   parse JSON to JavaScript objects                   │ │
│  │   return { products: [...], rentals: [...] }         │ │
│  │ }                                                     │ │
│  │ ↓                                                      │ │
│  │ Check if inventory.products has same ID              │ │
│  │ IF exists: return { success: false, error: '...' }   │ │
│  │ ↓                                                      │ │
│  │ inventory.products.push({id, name, price, qty})      │ │
│  │ ↓                                                      │ │
│  │ writeInventory(inventory) {                           │ │
│  │   convert to JSON string                             │ │
│  │   write to inventory.json file                       │ │
│  │   return true/false                                  │ │
│  │ }                                                     │ │
│  │ ↓                                                      │ │
│  │ Return { success: true, message: 'Product added' }   │ │
│  └────────────────────────────────────────────────────────┘ │
│                        ↓ Return to Server
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  FRONTEND (receives response)                               │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ const result = await res.json()                       │ │
│  │ ↓                                                      │ │
│  │ if (result.success) {                                 │ │
│  │   showSuccess("Product added!")                       │ │
│  │   await loadData()  // Fetch all products again      │ │
│  │   displayProducts() // Update table on page           │ │
│  │   updateCharts()    // Update dashboard charts        │ │
│  │ }                                                     │ │
│  └────────────────────────────────────────────────────────┘ │
│                        ↓ DOM Update
│  User sees new product in table and charts!                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## Learning Resources

This project demonstrates:
1. **C Data Structures** - Arrays, structs for real-world use
2. **Sorting Algorithms** - Bubble sort implementation
3. **Search Algorithms** - Linear search
4. **File I/O** - Reading/writing JSON files
5. **API Design** - RESTful endpoints
6. **Frontend-Backend Communication** - HTTP requests/responses
7. **Data Persistence** - JSON storage
8. **Full-Stack Development** - End-to-end system

---

## Troubleshooting

### **Port 8040 Already in Use**
```bash
docker ps
docker stop nexstock-c-backend-local
```

### **C Program Won't Compile**
```bash
# Make sure GCC is installed
gcc --version

# Build and run the C service
docker build -t nexstock-c-backend .
docker run --rm --env-file .env -p 8040:8040 nexstock-c-backend
```

### **Changes Not Showing**
```bash
# Clear browser cache
Ctrl+Shift+Delete

# Reload page
Ctrl+F5

# Restart the Docker container
docker restart nexstock-c-backend-local
```

---

## Example Workflow

### **Complete Scenario: Rental Management**

```
1. USER ADDS PRODUCTS
   Frontend: Add Laptop ($50,000), Mouse ($500)
   Backend: Creates inventory with 2 products

2. USER VIEWS DASHBOARD
   Frontend: Requests /api/c/products
   Backend: Reads inventory.json, returns products
   Frontend: Displays charts and statistics
   User sees: Total Value: ₹50,500, Total Qty: 2

3. USER RECORDS RENTAL
   Frontend: Fills rental form for Laptop
   Backend: 
     - Finds Laptop (quantity: 1)
     - Creates Rental struct
     - Decreases Laptop qty to 0
     - Saves rental to inventory.json
   Frontend: Shows rental recorded

4. USER CHECKS RENTALS
   Frontend: Requests /api/c/rentals
   Backend: Reads rentals array, returns all rentals
   Frontend: Shows active rental with one person

5. USER MARKS RENTAL RETURNED
   Frontend: Clicks "Return" on rental
   Backend:
     - Finds rental record
     - Updates status to "returned"
     - Finds Laptop, increases qty to 1
     - Saves to inventory.json
   Frontend: Shows rental as "RETURNED"
```

---

## Key Concepts

### **Why C Backend?**
- ✅ High performance for algorithms
- ✅ Memory efficient data structures
- ✅ Real-world industry standard
- ✅ Perfect for learning computer science fundamentals

### **Why Web Interface?**
- ✅ Beautiful user interface
- ✅ Cross-platform (works on any browser)
- ✅ Real-time charts and analytics
- ✅ Better user experience than CLI

### **Why JSON Storage?**
- ✅ Human readable
- ✅ Easy to parse
- ✅ Flexible schema
- ✅ No database setup needed

---

##  Performance Characteristics

| Operation | Time Complexity | Space Complexity |
|-----------|-----------------|------------------|
| Add Product | O(n) | O(1) |
| Delete Product | O(n) | O(1) |
| Search Product | O(n) | O(1) |
| Sort by ID/Name/Price | O(n²) | O(1) |
| Get All Products | O(1) | O(n) |
| Record Rental | O(n) | O(1) |

---

##  Verification Checklist

- [x] C backend logic implemented
- [x] C server routing working
- [x] Frontend using only C backend
- [x] MongoDB persistence
- [x] Signup, login, and bearer authentication
- [x] All CRUD operations functional
- [x] Sorting algorithms implemented
- [x] Search functionality working
- [x] Rental management complete
- [x] Charts and analytics displaying
- [x] Error handling implemented

---

##  Support

For issues or questions:
1. Check Docker logs for error messages
2. Verify `MONGODB_URI` is valid and the database is reachable
3. Ensure port 8040 is available
4. Check that inventory-api.c is present in the backend folder

---

##  Developer Info

**Project Name:** NexStock - Inventory Management System  
**Developer:** Naren S J  
**Date:** December 2025  
**Language:** C + JavaScript + HTML/CSS  
**Framework:** C HTTP server (libmicrohttpd)
**Database:** MongoDB (`products`, `rentals`, and `users` collections)
