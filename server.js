const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');

const PORT = 8080;
const INVENTORY_FILE = path.join(__dirname, 'backend', 'inventory.json');

// Utility function to read inventory.json
function readInventory() {
    try {
        const data = fs.readFileSync(INVENTORY_FILE, 'utf8');
        const parsed = JSON.parse(data || '{}');
        // Support both old format (array) and new format (object with products array)
        if (Array.isArray(parsed)) {
            return { products: parsed, rentals: [] };
        }
        return parsed;
    } catch (error) {
        console.error('Error reading inventory:', error);
        return { products: [], rentals: [] };
    }
}

// Utility function to write inventory.json
function writeInventory(inventoryData) {
    try {
        fs.writeFileSync(INVENTORY_FILE, JSON.stringify(inventoryData, null, 2), 'utf8');
        return true;
    } catch (error) {
        console.error('Error writing inventory:', error);
        return false;
    }
}

// Parse request body
function parseBody(req, callback) {
    let body = '';
    req.on('data', chunk => {
        body += chunk.toString();
    });
    req.on('end', () => {
        try {
            const data = body ? JSON.parse(body) : {};
            callback(null, data);
        } catch (error) {
            callback(error, null);
        }
    });
}

const server = http.createServer((req, res) => {
    const parsedUrl = url.parse(req.url, true);
    const pathname = parsedUrl.pathname;

    // CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    res.setHeader('Content-Type', 'application/json');

    if (req.method === 'OPTIONS') {
        res.writeHead(200);
        res.end();
        return;
    }

    // ==========================================
    // API ENDPOINTS FOR BACKEND COMMUNICATION
    // ==========================================

    // GET all products
    if (pathname === '/api/products' && req.method === 'GET') {
        const inventory = readInventory();
        res.writeHead(200);
        res.end(JSON.stringify(inventory.products));
        return;
    }

    // POST - Add new product
    if (pathname === '/api/products' && req.method === 'POST') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { id, name, price, quantity } = data;
            
            if (!id || !name || price === undefined || quantity === undefined) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Missing required fields' }));
                return;
            }

            const inventory = readInventory();
            
            if (inventory.products.find(p => p.id === parseInt(id))) {
                res.writeHead(409);
                res.end(JSON.stringify({ error: 'Product ID already exists' }));
                return;
            }

            inventory.products.push({
                id: parseInt(id),
                name: name.trim(),
                price: parseFloat(price),
                quantity: parseInt(quantity)
            });

            if (writeInventory(inventory)) {
                res.writeHead(201);
                res.end(JSON.stringify({ success: true, message: 'Product added' }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to save product' }));
            }
        });
        return;
    }

    // PUT - Update product
    if (pathname === '/api/products' && req.method === 'PUT') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { id, name, price, quantity } = data;
            
            if (!id) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Product ID required' }));
                return;
            }

            let inventory = readInventory();
            const productIndex = inventory.products.findIndex(p => p.id === parseInt(id));

            if (productIndex === -1) {
                res.writeHead(404);
                res.end(JSON.stringify({ error: 'Product not found' }));
                return;
            }

            inventory.products[productIndex] = {
                id: parseInt(id),
                name: name !== undefined ? name.trim() : inventory.products[productIndex].name,
                price: price !== undefined ? parseFloat(price) : inventory.products[productIndex].price,
                quantity: quantity !== undefined ? parseInt(quantity) : inventory.products[productIndex].quantity
            };

            if (writeInventory(inventory)) {
                res.writeHead(200);
                res.end(JSON.stringify({ success: true, message: 'Product updated' }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to update product' }));
            }
        });
        return;
    }

    // DELETE - Delete product
    if (pathname === '/api/products' && req.method === 'DELETE') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { id } = data;
            
            if (!id) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Product ID required' }));
                return;
            }

            let inventory = readInventory();
            const filteredProducts = inventory.products.filter(p => p.id !== parseInt(id));

            if (filteredProducts.length === inventory.products.length) {
                res.writeHead(404);
                res.end(JSON.stringify({ error: 'Product not found' }));
                return;
            }

            inventory.products = filteredProducts;

            if (writeInventory(inventory)) {
                res.writeHead(200);
                res.end(JSON.stringify({ success: true, message: 'Product deleted' }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to delete product' }));
            }
        });
        return;
    }

    // GET single product by ID
    if (pathname.startsWith('/api/products/') && req.method === 'GET') {
        const id = parseInt(pathname.split('/')[3]);
        const inventory = readInventory();
        const product = inventory.products.find(p => p.id === id);

        if (!product) {
            res.writeHead(404);
            res.end(JSON.stringify({ error: 'Product not found' }));
            return;
        }

        res.writeHead(200);
        res.end(JSON.stringify(product));
        return;
    }

    // POST - Sell product (reduce quantity)
    if (pathname === '/api/sell' && req.method === 'POST') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { productId, quantitySold } = data;
            
            if (!productId || !quantitySold || quantitySold <= 0) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Product ID and valid quantity required' }));
                return;
            }

            let inventory = readInventory();
            const productIndex = inventory.products.findIndex(p => p.id === parseInt(productId));

            if (productIndex === -1) {
                res.writeHead(404);
                res.end(JSON.stringify({ error: 'Product not found' }));
                return;
            }

            if (inventory.products[productIndex].quantity < quantitySold) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Insufficient quantity available' }));
                return;
            }

            inventory.products[productIndex].quantity -= quantitySold;
            // Record sale in sales history
            if (!Array.isArray(inventory.sales)) inventory.sales = [];
            const saleRecord = {
                saleId: Date.now(),
                productId: inventory.products[productIndex].id,
                productName: inventory.products[productIndex].name,
                quantitySold,
                date: new Date().toISOString().slice(0, 10),
                amount: Number((inventory.products[productIndex].price * quantitySold).toFixed(2))
            };
            inventory.sales.push(saleRecord);

            if (writeInventory(inventory)) {
                res.writeHead(200);
                res.end(JSON.stringify({ 
                    success: true, 
                    message: 'Product sold successfully',
                    product: inventory.products[productIndex],
                    sale: saleRecord
                }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to save changes' }));
            }
        });
        return;
    }

    // POST - Rent product
    if (pathname === '/api/rent' && req.method === 'POST') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { productId, renterName, returnDate, phoneNumber, address, amountPaid } = data;
            
            if (!productId || !renterName || !returnDate || !phoneNumber || !address || amountPaid === undefined) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'All rental fields are required' }));
                return;
            }

            let inventory = readInventory();
            const productIndex = inventory.products.findIndex(p => p.id === parseInt(productId));

            if (productIndex === -1) {
                res.writeHead(404);
                res.end(JSON.stringify({ error: 'Product not found' }));
                return;
            }

            // Ensure product is available for rent
            if (inventory.products[productIndex].quantity <= 0) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Product not available for rent' }));
                return;
            }

            // Decrease available quantity by 1 for rental
            inventory.products[productIndex].quantity = Math.max(0, inventory.products[productIndex].quantity - 1);

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

            inventory.rentals.push(rental);

            if (writeInventory(inventory)) {
                res.writeHead(201);
                res.end(JSON.stringify({ 
                    success: true, 
                    message: 'Rental recorded successfully',
                    rental: rental,
                    product: inventory.products[productIndex]
                }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to save rental' }));
            }
        });
        return;
    }

    // GET all rentals
    if (pathname === '/api/rentals' && req.method === 'GET') {
        const inventory = readInventory();
        const rentals = inventory.rentals || [];
        res.writeHead(200);
        res.end(JSON.stringify(rentals));
        return;
    }

    // PUT - Return rental (mark as returned)
    if (pathname === '/api/rentals/return' && req.method === 'PUT') {
        parseBody(req, (err, data) => {
            if (err) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Invalid JSON' }));
                return;
            }

            const { rentalId } = data;
            
            if (!rentalId) {
                res.writeHead(400);
                res.end(JSON.stringify({ error: 'Rental ID required' }));
                return;
            }

            let inventory = readInventory();
            const rentalIndex = (inventory.rentals || []).findIndex(r => r.rentalId === parseInt(rentalId));

            if (rentalIndex === -1) {
                res.writeHead(404);
                res.end(JSON.stringify({ error: 'Rental not found' }));
                return;
            }

            // Only process if currently active
            if (inventory.rentals[rentalIndex].status === 'returned') {
                res.writeHead(200);
                res.end(JSON.stringify({ success: true, message: 'Rental already returned' }));
                return;
            }

            inventory.rentals[rentalIndex].status = 'returned';
            inventory.rentals[rentalIndex].returnedDate = new Date().toISOString().split('T')[0];

            // Increase product quantity back by 1 if the product exists
            const pid = inventory.rentals[rentalIndex].productId;
            const productIndex = inventory.products.findIndex(p => p.id === parseInt(pid));
            if (productIndex !== -1) {
                inventory.products[productIndex].quantity = (inventory.products[productIndex].quantity || 0) + 1;
            }

            if (writeInventory(inventory)) {
                res.writeHead(200);
                res.end(JSON.stringify({ success: true, message: 'Rental marked as returned' }));
            } else {
                res.writeHead(500);
                res.end(JSON.stringify({ error: 'Failed to update rental' }));
            }
        });
        return;
    }

    // ==========================================
    // SERVE STATIC FILES (FRONTEND)
    // ==========================================

    if (pathname === '/backend/inventory.json') {
        const inventory = readInventory();
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(inventory.products));
        return;
    }

    let filePath = pathname === '/' ? '/frontend/index.html' : `/frontend${pathname}`;
    filePath = path.join(__dirname, filePath);

    const normalizedPath = path.normalize(filePath);
    if (!normalizedPath.startsWith(path.normalize(path.join(__dirname, 'frontend')))) {
        res.writeHead(403);
        res.end('Forbidden');
        return;
    }

    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            if (err.code === 'ENOENT' && path.extname(filePath) === '') {
                fs.readFile(path.join(__dirname, 'frontend', 'index.html'), 'utf8', (err2, data2) => {
                    if (err2) {
                        res.writeHead(404);
                        res.end('File not found');
                        return;
                    }
                    res.writeHead(200, { 'Content-Type': 'text/html' });
                    res.end(data2);
                });
                return;
            }

            res.writeHead(404);
            res.end('File not found');
            return;
        }

        const ext = path.extname(filePath);
        let contentType = 'text/html';
        
        switch (ext) {
            case '.js':
                contentType = 'application/javascript';
                break;
            case '.css':
                contentType = 'text/css';
                break;
            case '.json':
                contentType = 'application/json';
                break;
            case '.png':
                contentType = 'image/png';
                break;
            case '.jpg':
            case '.jpeg':
                contentType = 'image/jpeg';
                break;
            case '.svg':
                contentType = 'image/svg+xml';
                break;
        }

        res.writeHead(200, { 'Content-Type': contentType });
        res.end(data);
    });
});

server.listen(PORT, () => {
    console.log(`\n NexStock Server with Full API running at http://localhost:${PORT}`);
    console.log(` Frontend: http://localhost:${PORT}/`);
    console.log(`Dashboard: http://localhost:${PORT}/dashboard.html`);
    console.log(`Products: http://localhost:${PORT}/products.html`);
    console.log(`\n API Endpoints:`);
    console.log(`   GET    /api/products         - Get all products`);
    console.log(`   POST   /api/products         - Add new product`);
    console.log(`   PUT    /api/products         - Update product`);
    console.log(`   DELETE /api/products         - Delete product`);
    console.log(`   GET    /api/products/:id     - Get product by ID`);
    console.log(`   POST   /api/sell             - Sell product (reduce quantity)`);
    console.log(`   POST   /api/rent             - Record rental transaction`);
    console.log(`   GET    /api/rentals          - Get all rentals`);
    console.log(`   PUT    /api/rentals/return   - Mark rental as returned`);
    console.log(`\nData is automatically saved to backend/inventory.json`);
    console.log(` C backend can read this file anytime`);
    console.log(`\nPress Ctrl+C to stop the server\n`);
});
