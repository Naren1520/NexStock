// Apply saved theme immediately on page load
const savedTheme = localStorage.getItem('theme') || 'light';
if (savedTheme === 'dark') {
    document.documentElement.classList.add('dark-mode');
    document.body.classList.add('dark-mode');
}

// Initialize profile icon on all pages
document.addEventListener('DOMContentLoaded', function() {
    // Reapply theme in case it wasn't applied in time
    const theme = localStorage.getItem('theme') || 'light';
    if (theme === 'dark') {
        document.documentElement.classList.add('dark-mode');
        document.body.classList.add('dark-mode');
    }
    
    initializeProfileIcon();
    applyThemeToAllElements();
});

function applyThemeToAllElements() {
    const theme = localStorage.getItem('theme') || 'light';
    if (theme === 'dark') {
        // Apply dark mode to all potential elements
        document.querySelectorAll('.container, .header, .card, .modal-content').forEach(el => {
            if (!el.classList.contains('dark-mode')) {
                el.classList.add('dark-mode');
            }
        });
    }
}

function initializeProfileIcon() {
    // Create profile icon container
    const container = document.createElement('div');
    container.className = 'profile-icon-container';
    container.innerHTML = `
        <div class="profile-icon" onclick="toggleProfilePopup()">
            <span id="profileIconContent">👤</span>
        </div>
        <div class="profile-popup" id="profilePopup">
            <div class="profile-popup-header">
                <div class="profile-image-small" id="popupProfileImage">
                    <span>👤</span>
                </div>
                <div class="profile-popup-name" id="popupName">Not Set</div>
                <div class="profile-popup-org" id="popupOrg">No Organisation</div>
            </div>
            <a href="settings.html" class="profile-popup-link">⚙️ Edit Profile</a>
        </div>
    `;
    
    document.body.appendChild(container);
    
    // Load and display profile
    loadProfileData();
    
    // Close popup when clicking outside
    document.addEventListener('click', function(event) {
        const popup = document.getElementById('profilePopup');
        const icon = document.querySelector('.profile-icon');
        if (!event.target.closest('.profile-icon-container')) {
            popup.classList.remove('active');
        }
    });
}

function toggleProfilePopup() {
    const popup = document.getElementById('profilePopup');
    popup.classList.toggle('active');
}

function loadProfileData() {
    const profileData = localStorage.getItem('userProfile');
    const profileImage = localStorage.getItem('profileImage');
    
    if (profileImage) {
        // Update icon with image
        document.getElementById('profileIconContent').innerHTML = `<img src="${profileImage}" alt="Profile">`;
        // Update popup with image
        document.getElementById('popupProfileImage').innerHTML = `<img src="${profileImage}" alt="Profile">`;
    }
    
    if (profileData) {
        const profile = JSON.parse(profileData);
        const name = profile.fullName || 'Not Set';
        const org = profile.organisation || 'No Organisation';
        
        document.getElementById('popupName').textContent = name;
        document.getElementById('popupOrg').textContent = org;
    }
}
