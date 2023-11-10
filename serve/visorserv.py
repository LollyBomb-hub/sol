from fastapi import FastAPI, Request

from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker


# Простенький сервер

app = FastAPI()



@app.route("/{full_path:path}")
def capture_routes(request: Request, full_path: str):
    print(request.client.host, request.body(), full_path)

@app.post("/ua")
def write_to_db(request: Request):
    client_host = request.client.host
    body = request.body()
    print(body, client_host)
