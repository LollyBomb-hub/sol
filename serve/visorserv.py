from fastapi import FastAPI, Request

from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, Session
from sqlalchemy import Integer, String, Column, create_engine, ForeignKey
from sqlalchemy.dialects.postgresql import UUID

import uuid


Base = declarative_base()

class ActivityEntry(Base):
    __tablename__ = "activity_entry"

    id = Column(UUID(as_uuid=True), primary_key=True)
    domain = Column(String, index=True)
    computername = Column(String, index=True)
    username = Column(String, index=True)
    hostname = Column(String, index=True)


# Простенький сервер

app = FastAPI()

DBURL = "postgresql://postgres:password@localhost:25432/test"

engine = create_engine(DBURL, echo=True)
sess = Session(engine)

Base.metadata.create_all(engine)


@app.route("/{full_path:path}")
def capture_routes(request: Request, full_path: str):
    print(request.client.host, request.body(), full_path)

@app.post("/ua")
async def write_to_db(request: Request):
    client_host = request.client.host
    body = await request.body()
    print(len(body))
    u = uuid.uuid4()
    a=0
    for _ in range(3):
        a = body.index(b'\n', a + 1)
    f = body[a + 1:]
    r = body[:a].split(b'\n')
    r[0] = r[0].decode("ascii").replace("\x00", "")
    r[1] = r[1].decode("ascii").replace("\x00", "")
    r[2] = r[2].decode("ascii").replace("\x00", "")
    ae = ActivityEntry()
    ae.id = u
    ae.domain = r[0]
    ae.computername = r[1]
    ae.username = r[2]
    ae.hostname = client_host
    sess.add(ae)
    sess.commit()
    try:
        # Предлагается использовать garbage collector для старых фоток. В тз не входит
        with open(f"{u}.png", 'wb') as screenshot:
            screenshot.write(f)
    except:
        pass
