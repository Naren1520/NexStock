// C Backend Integration Module
// This provides functions to call C backend operations
// The C program reads and writes inventory.json directly

const fs = require('fs');
const path = require('path');

const INVENTORY_FILE = path.join(__dirname, 'inventory.json');

/**
 * Read inventory data from the shared JSON file
 */
function readInventory() {
    try {
        const data = fs.readFileSync(INVENTORY_FILE, 'utf8');
        const parsed = JSON.parse(data || '{}');
        if (Array.isArray(parsed)) {
            return { products: parsed, rentals: [] };
        }
        return parsed;
    } catch (error) {
        console.error('Error reading inventory:', error);
        return { products: [], rentals: [] };
    }
}

/**
 * Write inventory data to the shared JSON file
 */
function writeInventory(inventoryData) {
    try {
        fs.writeFileSync(INVENTORY_FILE, JSON.stringify(inventoryData, null, 2), 'utf8');
        return true;
    } catch (error) {
        console.error('Error writing inventory:', error);
        return false;
    }
}

/**
 * Add a product using C backend logic
 */
function addProduct(id, name, price, quantity) {
    const inventory = readInventory();
    
    // Check if ID already exists 
    if (inventory.products.find(p => p.id === parseInt(id))) {
        return { success: false, error: 'Product ID already exists' };
    }

    inventory.products.push({
        id: parseInt(id),
        name: name.trim(),
        price: parseFloat(price),
        quantity: parseInt(quantity)
    });

    if (writeInventory(inventory)) {
        return { success: true, message: 'Product added' };
    } else {
        return { success: false, error: 'Failed to save product' };
    }
}

/**
 * Delete a product using C backend logic
 */
function deleteProduct(id) {
    const inventory = readInventory();
    const initialLength = inventory.products.length;
    
    inventory.products = inventory.products.filter(p => p.id !== parseInt(id));

    if (inventory.products.length === initialLength) {
        return { success: false, error: 'Product not found' };
    }

    if (writeInventory(inventory)) {
        return { success: true, message: 'Product deleted' };
    } else {
        return { success: false, error: 'Failed to delete product' };
    }
}


//Update a product using C backend logic

function updateProduct(id, name, price, quantity) {
    const inventory = readInventory();
    const productIndex = inventory.products.findIndex(p => p.id === parseInt(id));

    if (productIndex === -1) {
        return { success: false, error: 'Product not found' };
    }

    inventory.products[productIndex] = {
        id: parseInt(id),
        name: name !== undefined ? name.trim() : inventory.products[productIndex].name,
        price: price !== undefined ? parseFloat(price) : inventory.products[productIndex].price,
        quantity: quantity !== undefined ? parseInt(quantity) : inventory.products[productIndex].quantity
    };

    if (writeInventory(inventory)) {
        return { success: true, message: 'Product updated' };
    } else {
        return { success: false, error: 'Failed to update product' };
    }
}


//Sell a product

function sellProduct(id, qty) {
    const inventory = readInventory();
    const productIndex = inventory.products.findIndex(p => p.id === parseInt(id));

    if (productIndex === -1) {
        return { success: false, error: 'Product not found' };
    }

    if (inventory.products[productIndex].quantity < qty) {
        return { success: false, error: 'Insufficient quantity', available: inventory.products[productIndex].quantity };
    }

    inventory.products[productIndex].quantity -= qty;

    if (writeInventory(inventory)) {
        return { success: true, quantity_remaining: inventory.products[productIndex].quantity };
    } else {
        return { success: false, error: 'Failed to process sale' };
    }
}

/**
 * Sort products by ID 
 */
function sortByID() {
    const inventory = readInventory();
    inventory.products.sort((a, b) => a.id - b.id);
    writeInventory(inventory);
    return { success: true, message: 'Sorted by ID', products: inventory.products };
}

/**
 * Sort products by name (C backend algorithm)
 */
function sortByName() {
    const inventory = readInventory();
    inventory.products.sort((a, b) => a.name.localeCompare(b.name));
    writeInventory(inventory);
    return { success: true, message: 'Sorted by Name', products: inventory.products };
}

/**
 * Sort products by price
 */
function sortByPrice() {
    const inventory = readInventory();
    inventory.products.sort((a, b) => a.price - b.price);
    writeInventory(inventory);
    return { success: true, message: 'Sorted by Price', products: inventory.products };
}

/**
 * Search product by ID 
 */
function searchProduct(id) {
    const inventory = readInventory();
    const product = inventory.products.find(p => p.id === parseInt(id));
    
    if (!product) {
        return { success: false, error: 'Product not found' };
    }

    return { success: true, product: product };
}

/**
 * Record a rental
 */
function recordRental(productId, renterName, returnDate, phoneNumber, address, amountPaid) {
    const inventory = readInventory();
    const productIndex = inventory.products.findIndex(p => p.id === parseInt(productId));

    if (productIndex === -1) {
        return { success: false, error: 'Product not found' };
    }

    if (inventory.products[productIndex].quantity <= 0) {
        return { success: false, error: 'Product not available for rent' };
    }

    // Decrease quantity
    inventory.products[productIndex].quantity--;

    const rental = {
        rentalId: Date.now(),
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

    if (!Array.isArray(inventory.rentals)) {
        inventory.rentals = [];
    }
    inventory.rentals.push(rental);

    if (writeInventory(inventory)) {
        return { success: true, message: 'Rental recorded', rental: rental };
    } else {
        return { success: false, error: 'Failed to save rental' };
    }
}

/**
 * Mark rental as returned
 */
function markRentalReturned(rentalId) {
    const inventory = readInventory();
    const rentalIndex = (inventory.rentals || []).findIndex(r => r.rentalId === parseInt(rentalId));

    if (rentalIndex === -1) {
        return { success: false, error: 'Rental not found' };
    }

    if (inventory.rentals[rentalIndex].status === 'returned') {
        return { success: true, message: 'Rental already returned' };
    }

    inventory.rentals[rentalIndex].status = 'returned';
    inventory.rentals[rentalIndex].returnedDate = new Date().toISOString().split('T')[0];

    // Return product to inventory
    const productId = inventory.rentals[rentalIndex].productId;
    const productIndex = inventory.products.findIndex(p => p.id === parseInt(productId));
    if (productIndex !== -1) {
        inventory.products[productIndex].quantity++;
    }

    if (writeInventory(inventory)) {
        return { success: true, message: 'Rental marked as returned' };
    } else {
        return { success: false, error: 'Failed to update rental' };
    }
}

/**
 * Get all rentals
 */
function getAllRentals() {
    const inventory = readInventory();
    return inventory.rentals || [];
}

module.exports = {
    readInventory,
    writeInventory,
    addProduct,
    deleteProduct,
    updateProduct,
    sellProduct,
    sortByID,
    sortByName,
    sortByPrice,
    searchProduct,
    recordRental,
    markRentalReturned,
    getAllRentals
};
