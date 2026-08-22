(function () {
    const originalFetch = window.fetch.bind(window);
    const apiBaseUrl = (window.NEXSTOCK_API_BASE_URL || "").replace(/\/$/, "");
    const publicPaths = ["/login.html", "/api/auth/", "/api/chatbot-key"];
    const isPublic = () => publicPaths.some((path) => window.location.pathname.endsWith(path) || window.location.pathname.startsWith(path));

    if (!isPublic() && !localStorage.getItem("nexstock_token")) {
        window.location.replace("login.html");
        return;
    }

    window.fetch = function (resource, options) {
        const requestOptions = options ? { ...options, headers: new Headers(options.headers || {}) } : { headers: new Headers() };
        const token = localStorage.getItem("nexstock_token");
        if (token) requestOptions.headers.set("Authorization", `Bearer ${token}`);
        const requestUrl = typeof resource === "string" && apiBaseUrl && resource.startsWith("/api/")
            ? `${apiBaseUrl}${resource}`
            : resource;
        return originalFetch(requestUrl, requestOptions).then((response) => {
            if (response.status === 401 && !isPublic()) {
                localStorage.removeItem("nexstock_token");
                localStorage.removeItem("nexstock_user");
                window.location.replace("login.html");
            }
            return response;
        });
    };

    window.nexstockAuth = {
        save(session) {
            localStorage.setItem("nexstock_token", session.token);
            localStorage.setItem("nexstock_user", typeof session.user === "string" ? session.user : JSON.stringify(session.user));
        },
        logout() {
            localStorage.removeItem("nexstock_token");
            localStorage.removeItem("nexstock_user");
            window.location.replace("login.html");
        }
    };
})();
