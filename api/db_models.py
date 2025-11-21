from database import get_connection

def create_tables():
	conn = get_connection()
	c = conn.cursor()
	c.execute('''
		CREATE TABLE IF NOT EXISTS sensor_data (
			id INTEGER PRIMARY KEY AUTOINCREMENT, 
			sensor_type TEXT,
			value TEXT, 
			timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
			)
		''')
	conn.commit()
	conn.close()
