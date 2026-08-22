# NexStock

Inventory and rental management application with a C HTTP backend, MongoDB persistence, and a browser frontend.

## Architecture

- `backend/inventory-api.c`: C HTTP server, authentication, inventory and rental API
- `frontend/`: static HTML, CSS, and JavaScript application
- MongoDB: `users`, `products`, and `rentals` collections
- `Dockerfile`: builds and runs the C service
- `render.yaml`: Render deployment configuration
- `vercel.json`: optional Vercel frontend proxy configuration

There is no Node.js backend. The browser JavaScript is used for the UI and AI chatbot only.

## Requirements

- Docker Desktop
- MongoDB Atlas or another reachable MongoDB deployment
- Git

## Configuration

Copy `.env.example` to `.env` and set the values locally:

```env
MONGODB_URI=mongodb+srv://username:password@cluster.example.mongodb.net/?retryWrites=true&w=majority
MONGODB_DATABASE=nexstock
GEMINI_API_KEY=your_gemini_api_key
PORT=8040
```

Do not commit `.env`. Render supplies `PORT` automatically. Configure `MONGODB_URI`, `MONGODB_DATABASE`, and `GEMINI_API_KEY` as Render environment variables.

## Run Locally

```bash
docker build -t nexstock-c-backend .
docker run --rm --name nexstock-c-backend-local --env-file .env -p 8040:8040 nexstock-c-backend
```

Open <http://localhost:8040/>. Unauthenticated users are sent to `/login.html`.

To stop a detached container:

```bash
docker stop nexstock-c-backend-local
```

## Authentication

Users can sign up and log in with an email address and password. Passwords are stored as SHA-256 hashes in MongoDB. Inventory and rental endpoints require a bearer token.

Public endpoints:

- `POST /api/auth/signup`
- `POST /api/auth/login`
- `GET /api/chatbot-key`

Sessions are held in memory and are invalidated when the C service restarts.

## API

Product endpoints:

- `GET /api/c/products`
- `POST /api/c/product/add`
- `PUT /api/c/product/update`
- `DELETE /api/c/product/delete`
- `POST /api/c/product/sell`
- `GET /api/c/product/search/:id`
- `GET /api/c/product/sort/id`
- `GET /api/c/product/sort/name`
- `GET /api/c/product/sort/price`

Rental endpoints:

- `GET /api/c/rentals`
- `POST /api/c/rental/record`
- `PUT /api/c/rental/return`

## Deployment

### Render Backend

1. Push the repository to GitHub.
2. Create a Render Web Service from the repository.
3. Select the Docker runtime.
4. Add the environment variables listed above.
5. Deploy using `Dockerfile` or sync `render.yaml`.

### Vercel Frontend

The repository includes `vercel.json`, which proxies `/api/*` to the Render backend. Set the Vercel project root to the repository root, use the `Other` framework preset, and leave the build command empty. The frontend files are served from the repository's `frontend` directory through the C backend; for a separate Vercel static deployment, configure the output directory as `frontend`.

Update the Render URL in `vercel.json` if the backend service URL changes.

## Data Migration

`backend/inventory.json` is retained as legacy seed/reference data. The running API reads and writes MongoDB and does not automatically import this file. Import required products and rentals into MongoDB before production use.

## Troubleshooting

- Check service logs with `docker logs nexstock-c-backend-local`.
- Confirm MongoDB Atlas network access allows the deployment.
- Confirm the MongoDB user has read and write permissions.
- If the browser shows connection refused, verify the container is running and port `8040` is available.
- If the browser shows an old page, reload without cache or restart the container after rebuilding the image.
