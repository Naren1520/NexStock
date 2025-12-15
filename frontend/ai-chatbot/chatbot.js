
class AIChapbot {
    constructor() {
        this.apiKey = null;
        this.messages = [];
        this.isLoading = false;
        this.geminiApiUrl = 'https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent';
        this.init();
    }

    async init() {
        // Fetch API key from backend
        try {
            const response = await fetch('/api/chatbot-key');
            const data = await response.json();
            this.apiKey = data.apiKey;
        } catch (error) {
            console.error('Failed to fetch API key:', error);
        }

        this.createChatbotUI();
        this.attachEventListeners();
    }

    createChatbotUI() {
        // Check if chatbot already exists
        if (document.getElementById('chatbot-container')) {
            return;
        }

        const chatbotHTML = `
            <div class="chatbot-container" id="chatbot-container">
                <button class="chatbot-toggle" id="chatbot-toggle" title="Open AI Chatbot">
                    <img src="assests/logo.png" alt="Chatbot" onerror="this.src='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 24 24%22 fill=%22white%22%3E%3Cpath d=%22M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-2 15l-5-5 1.41-1.41L10 14.17l7.59-7.59L19 8l-9 9z%22/%3E%3C/svg%3E'">
                </button>
                <div class="chatbot-window" id="chatbot-window">
                    <div class="chatbot-header">
                        <div class="chatbot-header-left">
                            <img class="chatbot-header-logo" src="assests/logo.png" alt="Logo" onerror="this.style.display='none'">
                            <h3>AI Assistant</h3>
                        </div>
                        <button class="chatbot-close" id="chatbot-close">×</button>
                    </div>
                    <div class="chatbot-messages" id="chatbot-messages"></div>
                    <div class="chatbot-input-area">
                        <input type="text" class="chatbot-input" id="chatbot-input" placeholder="Type your message..." autocomplete="off">
                        <button class="chatbot-send" id="chatbot-send">➤</button>
                    </div>
                </div>
            </div>
        `;

        // Add chatbot CSS link to head
        if (!document.querySelector('link[href*="chatbot.css"]')) {
            const cssLink = document.createElement('link');
            cssLink.rel = 'stylesheet';
            cssLink.href = 'ai-chatbot/chatbot.css';
            document.head.appendChild(cssLink);
        }

        document.body.insertAdjacentHTML('beforeend', chatbotHTML);
    }

    attachEventListeners() {
        const toggle = document.getElementById('chatbot-toggle');
        const close = document.getElementById('chatbot-close');
        const input = document.getElementById('chatbot-input');
        const send = document.getElementById('chatbot-send');
        const window = document.getElementById('chatbot-window');

        if (toggle) toggle.addEventListener('click', () => this.toggleChatbot());
        if (close) close.addEventListener('click', () => this.closeChatbot());
        if (send) send.addEventListener('click', () => this.sendMessage());
        if (input) {
            input.addEventListener('keypress', (e) => {
                if (e.key === 'Enter') this.sendMessage();
            });
        }
    }

    toggleChatbot() {
        const window = document.getElementById('chatbot-window');
        if (window) {
            window.classList.toggle('open');
            if (window.classList.contains('open')) {
                // Show welcome message if no messages exist
                const messagesContainer = document.getElementById('chatbot-messages');
                if (messagesContainer.children.length === 0) {
                    this.addMessageToUI('bot', 'How can NexStock Ai Help You');
                }
                document.getElementById('chatbot-input').focus();
            }
        }
    }

    closeChatbot() {
        const window = document.getElementById('chatbot-window');
        if (window) {
            window.classList.remove('open');
        }
    }

    async sendMessage() {
        const input = document.getElementById('chatbot-input');
        const message = input.value.trim();

        if (!message) return;

        // Add user message to UI
        this.addMessageToUI('user', message);
        input.value = '';

        if (!this.apiKey) {
            this.addMessageToUI('bot', 'Error: API key not configured.');
            return;
        }

        // Show typing indicator
        this.showTypingIndicator();

        try {
            const response = await this.callGeminiAPI(message);
            this.removeTypingIndicator();
            this.addMessageToUI('bot', response);
        } catch (error) {
            this.removeTypingIndicator();
            this.addMessageToUI('bot', 'Sorry, I encountered an error. Please try again.');
            console.error('Chatbot error:', error);
        }
    }

    async callGeminiAPI(userMessage) {
        const requestBody = {
            contents: [
                {
                    parts: [
                        {
                            text: userMessage
                        }
                    ]
                }
            ],
            generationConfig: {
                temperature: 0.7,
                topK: 40,
                topP: 0.95,
                maxOutputTokens: 1024,
            },
            safetySettings: [
                {
                    category: "HARM_CATEGORY_HARASSMENT",
                    threshold: "BLOCK_MEDIUM_AND_ABOVE"
                },
                {
                    category: "HARM_CATEGORY_HATE_SPEECH",
                    threshold: "BLOCK_MEDIUM_AND_ABOVE"
                }
            ]
        };

        const response = await fetch(`${this.geminiApiUrl}?key=${this.apiKey}`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(requestBody)
        });

        if (!response.ok) {
            if (response.status === 401) {
                throw new Error('Invalid API key.');
            }
            throw new Error(`API error: ${response.statusText}`);
        }

        const data = await response.json();
        
        if (data.candidates && data.candidates.length > 0) {
            const textContent = data.candidates[0].content.parts
                .filter(part => part.text)
                .map(part => part.text)
                .join('');
            return textContent || 'No response generated.';
        }

        throw new Error('No response from API');
    }

    addMessageToUI(sender, text) {
        const messagesContainer = document.getElementById('chatbot-messages');
        if (!messagesContainer) return;

        const messageDiv = document.createElement('div');
        messageDiv.className = `message ${sender}`;
        
        const contentDiv = document.createElement('div');
        contentDiv.className = 'message-content';
        contentDiv.textContent = text;

        messageDiv.appendChild(contentDiv);
        messagesContainer.appendChild(messageDiv);

        // Scroll to bottom
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    showTypingIndicator() {
        const messagesContainer = document.getElementById('chatbot-messages');
        if (!messagesContainer) return;

        const messageDiv = document.createElement('div');
        messageDiv.className = 'message bot';
        messageDiv.id = 'typing-indicator';
        
        const contentDiv = document.createElement('div');
        contentDiv.className = 'message-content';
        
        const typingDiv = document.createElement('div');
        typingDiv.className = 'typing-indicator';
        typingDiv.innerHTML = '<span></span><span></span><span></span>';

        contentDiv.appendChild(typingDiv);
        messageDiv.appendChild(contentDiv);
        messagesContainer.appendChild(messageDiv);

        // Scroll to bottom
        messagesContainer.scrollTop = messagesContainer.scrollHeight;
    }

    removeTypingIndicator() {
        const indicator = document.getElementById('typing-indicator');
        if (indicator) {
            indicator.remove();
        }
    }
}

// Initialize chatbot when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
        new AIChapbot();
    });
} else {
    new AIChapbot();
}
