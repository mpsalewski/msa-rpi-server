import sqlite3

DB_FILE = "server.db"

def get_connection():
	conn = sqlite3.connect(DB_FILE,check_same_thread=False)
	return conn
	
	
	
def init_db():
	from db_models import create_tables
	create_tables()
