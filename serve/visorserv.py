from fastapi import FastAPI, Request

from sqlalchemy import create_engine
from sqlalchemy.engine import URL
from sqlalchemy import Column, Integer, String, DateTime, Text, LargeBinary
from sqlalchemy.orm import declarative_base
from datetime import datetime
from sqlalchemy.orm import sessionmaker


# Простенький сервер

app = FastAPI()

url = URL.create(
    drivername="drivername",
    username="username",
    host="host",
    database="dbname"
)

engine = create_engine(url)

Base = declarative_base()

class Client(Base):
    __tablename__ = 'clients'

    id = Column(Integer(), primary_key=True)
    host = Column(String(), index=True)
    domain = Column(String(), index=True)
    machine = Column(String(), index=True)
    username = Column(String(), index=True)
    photo = Column(LargeBinary())
    datetime = Column(DateTime(), default=datetime.now)

Base.metadata.create_all(engine)

Session = sessionmaker(bind=engine)
session = Session()

@app.route("/{full_path:path}")
def capture_routes(request: Request, full_path: str):
    print(request.client.host, request.body(), full_path)

@app.post("/ua")
def write_to_db(request: Request):
    client_host = request.client.host
    body = str(request.body())
    v = body.split('\n')
    client = Client(
        domain=v[0],
        machine=v[1],
        username=v[2],
        photo=v[3]
    )
    session.add(client)
    session.commit()
    print(body, client_host)
