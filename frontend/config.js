(function () {
    const localHosts = ["localhost", "127.0.0.1"];
    const isLocal = localHosts.includes(window.location.hostname);
    window.NEXSTOCK_API_BASE_URL = isLocal ? "" : "https://nexstock-9pnw.onrender.com";
})();
