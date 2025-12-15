//  Menu Toggle 
document.addEventListener('DOMContentLoaded', function() {
    const hamburgerBtn = document.querySelector('.hamburger-btn');
    const mobileNav = document.querySelector('.mobile-nav');
    const overlay = document.querySelector('.mobile-nav-overlay');
    const navLinks = document.querySelectorAll('.mobile-nav a, .mobile-nav button');

    // Toggle menu when hamburger is clicked
    if (hamburgerBtn) {
        hamburgerBtn.addEventListener('click', function() {
            hamburgerBtn.classList.toggle('active');
            mobileNav.classList.toggle('active');
            overlay.classList.toggle('active');
            document.body.style.overflow = mobileNav.classList.contains('active') ? 'hidden' : 'auto';
        });
    }

    // Close menu when overlay is clicked
    if (overlay) {
        overlay.addEventListener('click', function() {
            hamburgerBtn.classList.remove('active');
            mobileNav.classList.remove('active');
            overlay.classList.remove('active');
            document.body.style.overflow = 'auto';
        });
    }

    // Close menu when a link is clicked
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            // Only close if it's a navigation link 
            if (this.tagName === 'A' || !this.onclick) {
                hamburgerBtn.classList.remove('active');
                mobileNav.classList.remove('active');
                overlay.classList.remove('active');
                document.body.style.overflow = 'auto';
            }
        });
    });

    // Close menu on escape key
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape' && mobileNav.classList.contains('active')) {
            hamburgerBtn.classList.remove('active');
            mobileNav.classList.remove('active');
            overlay.classList.remove('active');
            document.body.style.overflow = 'auto';
        }
    });

    // Mark current page as active in mobile menu
    markActiveNavItem();
});

function markActiveNavItem() {
    const currentPath = window.location.pathname;
    const navItems = document.querySelectorAll('.mobile-nav a, .mobile-nav button');
    
    navItems.forEach(item => {
        if (item.tagName === 'A') {
            const href = item.getAttribute('href');
            if (currentPath.includes(href) || 
                (currentPath.endsWith('/') && href === 'main.html')) {
                item.classList.add('active');
            } else {
                item.classList.remove('active');
            }
        }
    });
}
