
// ==========================================
// NexStock
// Main Script File - Organized & Modular
// ==========================================

// ==========================================
// 1. GLOBAL VARIABLES & CHART INSTANCES
// ==========================================
let allProducts = [];
let priceChart, quantityChart, distributionChart, topPriceChart, topQuantityChart, valueChart;

// ==========================================
// 2. DATA LOADING & MANAGEMENT
// ==========================================

/**
 * Load products from inventory.json
 */
async function loadData() {
    try {
        const res = await fetch("../backend/inventory.json");
        if (!res.ok) throw new Error("Failed to load inventory");
        allProducts = await res.json();
        displayProducts();
        updateDashboard();
        updateCharts();
    } catch (error) {
        console.error("Error loading data:", error);
        showError("Failed to load inventory data");
    }
}

/**
 * Display all products in table format
 */
function displayProducts() {
    const tbody = document.getElementById("body");
    const emptyMsg = document.getElementById("empty-msg");

    if (allProducts.length === 0) {
        tbody.innerHTML = "";
        emptyMsg.style.display = "block";
        return;
    }

    emptyMsg.style.display = "none";
    tbody.innerHTML = allProducts.map(p => `
        <tr>
            <td>${p.id}</td>
            <td>${p.name}</td>
            <td>₹${p.price.toFixed(2)}</td>
            <td>${p.quantity}</td>
            <td>₹${(p.price * p.quantity).toFixed(2)}</td>
            <td>
                <button class="btn-info btn-sm" onclick="openEditModal(${p.id})">Edit</button>
                <button class="btn-danger btn-sm" onclick="deleteProduct(${p.id})">Delete</button>
            </td>
        </tr>
    `).join("");
}

// ==========================================
// 3. PRODUCT OPERATIONS (ADD, EDIT, DELETE)
// ==========================================

/**
 * Add a new product to inventory
 * @param {Event} e - Form submission event
 */
function addProduct(e) {
    e.preventDefault();
    const id = parseInt(document.getElementById("productId").value);
    const name = document.getElementById("productName").value.trim();
    const price = parseFloat(document.getElementById("productPrice").value);
    const quantity = parseInt(document.getElementById("productQuantity").value);

    // Validation
    if (!id || !name || price <= 0 || quantity <= 0) {
        showError("Please fill all fields correctly");
        return;
    }

    if (allProducts.find(p => p.id === id)) {
        showError("Product ID already exists!");
        return;
    }

    // Add product
    allProducts.push({ id, name, price, quantity });
    showSuccess("Product added successfully!");
    e.target.reset();
    displayProducts();
    updateDashboard();
    updateCharts();
}

/**
 * Open modal to edit a product
 * @param {number} id - Product ID
 */
function openEditModal(id) {
    const product = allProducts.find(p => p.id === id);
    if (product) {
        document.getElementById("editId").value = product.id;
        document.getElementById("editName").value = product.name;
        document.getElementById("editPrice").value = product.price;
        document.getElementById("editQuantity").value = product.quantity;
        document.getElementById("editModal").classList.add("active");
    }
}

/**
 * Save edited product changes
 */
function saveProduct() {
    const id = parseInt(document.getElementById("editId").value);
    const product = allProducts.find(p => p.id === id);

    if (product) {
        product.name = document.getElementById("editName").value.trim();
        product.price = parseFloat(document.getElementById("editPrice").value);
        product.quantity = parseInt(document.getElementById("editQuantity").value);
        
        displayProducts();
        updateDashboard();
        updateCharts();
        closeModal();
        showSuccess("Product updated successfully!");
    }
}

/**
 * Close edit modal
 */
function closeModal() {
    document.getElementById("editModal").classList.remove("active");
}

/**
 * Delete a product from inventory
 * @param {number} id - Product ID
 */
function deleteProduct(id) {
    if (confirm("Are you sure you want to delete this product?")) {
        allProducts = allProducts.filter(p => p.id !== id);
        displayProducts();
        updateDashboard();
        updateCharts();
        showSuccess("Product deleted successfully!");
    }
}

// ==========================================
// 4. SEARCH & FILTER OPERATIONS
// ==========================================

/**
 * Search product by ID and display result
 */
function searchByID() {
    const id = parseInt(document.getElementById("searchId").value);
    const product = allProducts.find(p => p.id === id);
    const resultDiv = document.getElementById("searchResult");

    if (product) {
        resultDiv.innerHTML = `
            <div class="card">
                <h3 style="color: #4caf50; margin-bottom: 20px;">✓ Product Found</h3>
                <table>
                    <tr><th>Property</th><th>Value</th></tr>
                    <tr><td>ID</td><td>${product.id}</td></tr>
                    <tr><td>Name</td><td>${product.name}</td></tr>
                    <tr><td>Price</td><td>₹${product.price.toFixed(2)}</td></tr>
                    <tr><td>Quantity</td><td>${product.quantity}</td></tr>
                        <tr><td>Total Value</td><td>₹${(product.price * product.quantity).toFixed(2)}</td></tr>
                </table>
            </div>
        `;
    } else {
        resultDiv.innerHTML = `
            <div class="card" style="background: #ffebee;">
                <h3 style="color: #f44336;">✗ Product Not Found</h3>
                <p>No product with ID ${id} found in inventory.</p>
            </div>
        `;
    }
}

/**
 * Search product by name in table
 */
function searchByName() {
    const searchTerm = document.getElementById("searchName").value.toLowerCase().trim();
    const resultDiv = document.getElementById("searchResultByName");

    if (!searchTerm) {
        resultDiv.innerHTML = '';
        return;
    }

    const results = allProducts.filter(p => p.name.toLowerCase().includes(searchTerm));

    if (results.length === 0) {
        resultDiv.innerHTML = `<p style="color: #999; margin-top: 20px;">No products found matching "${searchTerm}"</p>`;
        return;
    }

    let html = `<h3 style="margin-top: 30px;">Results for "${searchTerm}" (${results.length} found)</h3>
                <table style="margin-top: 15px;">
                    <thead><tr><th>ID</th><th>Name</th><th>Price</th><th>Quantity</th></tr></thead>
                    <tbody>`;
    
    results.forEach(p => {
        html += `<tr><td>${p.id}</td><td>${p.name}</td><td>₹${p.price.toFixed(2)}</td><td>${p.quantity}</td></tr>`;
    });
    
    html += '</tbody></table>';
    resultDiv.innerHTML = html;
}

/**
 * Search product by price
 */
function searchByPrice() {
    const price = parseFloat(document.getElementById("searchPrice").value);
    const resultDiv = document.getElementById("searchResultByPrice");

    if (!price || price <= 0) {
        resultDiv.innerHTML = '';
        return;
    }

    const results = allProducts.filter(p => Math.abs(p.price - price) < 0.01 || p.price === price);

    if (results.length === 0) {
        resultDiv.innerHTML = `<p style="color: #999; margin-top: 20px;">No products found with price ₹${price.toFixed(2)}</p>`;
        return;
    }

    let html = `<h3 style="margin-top: 30px;">Products with price ₹${price.toFixed(2)} (${results.length} found)</h3>
                <table style="margin-top: 15px;">
                    <thead><tr><th>ID</th><th>Name</th><th>Price</th><th>Quantity</th></tr></thead>
                    <tbody>`;
    
    results.forEach(p => {
        html += `<tr><td>${p.id}</td><td>${p.name}</td><td>$${p.price.toFixed(2)}</td><td>${p.quantity}</td></tr>`;
    });
    
    html += '</tbody></table>';
    resultDiv.innerHTML = html;
}

/**
 * Search in products table
 */
function searchProduct() {
    const filter = document.getElementById("search").value.toLowerCase();
    const rows = document.querySelectorAll("#table tbody tr");

    rows.forEach(row => {
        const text = row.innerText.toLowerCase();
        row.style.display = text.includes(filter) ? "" : "none";
    });
}

// ==========================================
// 5. SORTING OPERATIONS
// ==========================================

/**
 * Sort products by ID
 */
function sortByID() {
    allProducts.sort((a, b) => a.id - b.id);
    displayProducts();
}

/**
 * Sort products by Name
 */
function sortByName() {
    allProducts.sort((a, b) => a.name.localeCompare(b.name));
    displayProducts();
}

/**
 * Sort products by Price
 */
function sortByPrice() {
    allProducts.sort((a, b) => a.price - b.price);
    displayProducts();
}

// ==========================================
// 6. DASHBOARD STATISTICS
// ==========================================

/**
 * Update dashboard with inventory statistics
 */
function updateDashboard() {
    const total = allProducts.length;
    const totalValue = allProducts.reduce((sum, p) => sum + (p.price * p.quantity), 0);
    const totalQty = allProducts.reduce((sum, p) => sum + p.quantity, 0);
    const avgPrice = total > 0 ? allProducts.reduce((sum, p) => sum + p.price, 0) / total : 0;

    document.getElementById("totalProducts").textContent = total;
    document.getElementById("totalValue").textContent = `$${totalValue.toFixed(2)}`;
    document.getElementById("totalQuantity").textContent = totalQty;
    document.getElementById("avgPrice").textContent = `$${avgPrice.toFixed(2)}`;
}

// ==========================================
// 7. CHART OPERATIONS
// ==========================================

/**
 * Update all charts with current product data
 */
function updateCharts() {
    if (allProducts.length === 0) return;

    createPriceChart();
    createQuantityChart();
    createDistributionChart();
    createTopPriceChart();
    createTopQuantityChart();
    createValueChart();
}

/**
 * Create/Update Price Distribution Chart (Doughnut)
 */
function createPriceChart() {
    const priceCtx = document.getElementById("priceChart").getContext("2d");
    if (priceChart) priceChart.destroy();
    priceChart = new Chart(priceCtx, {
        type: "doughnut",
        data: {
            labels: allProducts.map(p => p.name),
            datasets: [{
                data: allProducts.map(p => p.price),
                backgroundColor: [
                    "#667eea", "#764ba2", "#f093fb", "#4facfe", "#00f2fe",
                    "#43e97b", "#fa709a", "#fee140", "#30b0fe", "#eb3b5a"
                ]
            }]
        },
        options: {
            responsive: true,
            plugins: {
                legend: { position: "right" },
                title: { display: true, text: "Product Prices Distribution" }
            }
        }
    });
}

/**
 * Create/Update Quantity Chart (Bar)
 */
function createQuantityChart() {
    const quantityCtx = document.getElementById("quantityChart").getContext("2d");
    if (quantityChart) quantityChart.destroy();
    quantityChart = new Chart(quantityCtx, {
        type: "bar",
        data: {
            labels: allProducts.map(p => p.name),
            datasets: [{
                label: "Quantity",
                data: allProducts.map(p => p.quantity),
                backgroundColor: "#4caf50"
            }]
        },
        options: {
            responsive: true,
            indexAxis: "y",
            plugins: {
                legend: { display: false },
                title: { display: true, text: "Product Quantities" }
            }
        }
    });
}

/**
 * Create/Update Distribution Chart (Pie)
 */
function createDistributionChart() {
    const distributionCtx = document.getElementById("distributionChart").getContext("2d");
    if (distributionChart) distributionChart.destroy();
    const priceRanges = ["₹0-50", "₹50-100", "₹100-200", "₹200+"];
    const priceCount = [0, 0, 0, 0];
    allProducts.forEach(p => {
        if (p.price < 50) priceCount[0]++;
        else if (p.price < 100) priceCount[1]++;
        else if (p.price < 200) priceCount[2]++;
        else priceCount[3]++;
    });
    distributionChart = new Chart(distributionCtx, {
        type: "pie",
        data: {
            labels: priceRanges,
            datasets: [{
                data: priceCount,
                backgroundColor: ["#667eea", "#764ba2", "#f093fb", "#fa709a"]
            }]
        },
        options: { responsive: true, plugins: { legend: { position: "bottom" } } }
    });
}

/**
 * Create/Update Top Price Chart (Horizontal Bar)
 */
function createTopPriceChart() {
    const sorted = [...allProducts].sort((a, b) => b.price - a.price).slice(0, 5);
    const topPriceCtx = document.getElementById("topPriceChart").getContext("2d");
    if (topPriceChart) topPriceChart.destroy();
    topPriceChart = new Chart(topPriceCtx, {
        type: "bar",
        data: {
            labels: sorted.map(p => p.name),
            datasets: [{
                label: "Price (₹)",
                data: sorted.map(p => p.price),
                backgroundColor: "#2196F3"
            }]
        },
        options: { indexAxis: "y", responsive: true, plugins: { legend: { display: false } } }
    });
}

/**
 * Create/Update Top Quantity Chart (Bar)
 */
function createTopQuantityChart() {
    const topQty = [...allProducts].sort((a, b) => b.quantity - a.quantity).slice(0, 5);
    const topQuantityCtx = document.getElementById("topQuantityChart").getContext("2d");
    if (topQuantityChart) topQuantityChart.destroy();
    topQuantityChart = new Chart(topQuantityCtx, {
        type: "bar",
        data: {
            labels: topQty.map(p => p.name),
            datasets: [{
                label: "Quantity",
                data: topQty.map(p => p.quantity),
                backgroundColor: "#ff9800"
            }]
        },
        options: { responsive: true, plugins: { legend: { display: false } } }
    });
}

/**
 * Create/Update Value Chart (Line)
 */
function createValueChart() {
    const valueCtx = document.getElementById("valueChart").getContext("2d");
    if (valueChart) valueChart.destroy();
    valueChart = new Chart(valueCtx, {
        type: "line",
        data: {
            labels: allProducts.map(p => p.name),
            datasets: [{
                label: "Inventory Value (₹)",
                data: allProducts.map(p => p.price * p.quantity),
                borderColor: "#667eea",
                backgroundColor: "rgba(102, 126, 234, 0.1)",
                borderWidth: 2,
                fill: true,
                tension: 0.4
            }]
        },
        options: { responsive: true, plugins: { legend: { display: true } } }
    });
}

// ==========================================
// 8. UI & MESSAGING
// ==========================================

/**
 * Show success message
 * @param {string} msg - Success message to display
 */
function showSuccess(msg) {
    const successMsg = document.getElementById("successMsg");
    if (successMsg) {
        successMsg.textContent = msg;
        successMsg.style.display = "block";
        setTimeout(() => successMsg.style.display = "none", 3000);
    }
}

/**
 * Show error message
 * @param {string} msg - Error message to display
 */
function showError(msg) {
    const errorMsg = document.getElementById("errorMsg");
    if (errorMsg) {
        errorMsg.textContent = msg;
        errorMsg.style.display = "block";
        setTimeout(() => errorMsg.style.display = "none", 3000);
    }
}

/**
 * Initialize tab switching functionality
 */
function initTabSwitching() {
    document.querySelectorAll(".tab-btn").forEach(btn => {
        btn.addEventListener("click", () => {
            document.querySelectorAll(".tab-btn").forEach(b => b.classList.remove("active"));
            document.querySelectorAll(".tab-content").forEach(t => t.classList.remove("active"));
            btn.classList.add("active");
            document.getElementById(btn.dataset.tab).classList.add("active");
        });
    });
}

// ==========================================
// 9. INITIALIZATION
// ==========================================

/**
 * Initialize application on page load
 */
document.addEventListener("DOMContentLoaded", function() {
    loadData();
    initTabSwitching();
});

// Load data on page load as fallback
loadData();