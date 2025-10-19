# animal_shelter.py
# Enhanced version for CS499 Milestone Four – Databases
# Kris Marchevka - CS499 - Computer Science Capstone

import os
from pymongo import MongoClient, ASCENDING, errors
from dotenv import load_dotenv

# Loads environment variables
load_dotenv()

class AnimalShelter:
    """ CRUD ops for Animal collection in MongoDB with validation, indexing, and aggregation """

    def __init__(self):
        try:
            self.client = MongoClient(os.getenv("DB_URI"))
            self.database = self.client[os.getenv("DB_NAME")]
            self.collection = self.database["animals"]

            # indexes for faster searches
            self.collection.create_index([("breed", ASCENDING)])
            self.collection.create_index([("animal_type", ASCENDING)])
            self.collection.create_index([("age_upon_outcome_in_weeks", ASCENDING)])
            print("Connected to MongoDB and verified indexes.")

        except errors.ConnectionFailure as e:
            print(f"ERROR! Could not connect to MongoDB: {e}")

    # CREATE QUERY
    def create(self, data: dict) -> bool:
        """Insert a new animal document into the collection."""
        if not isinstance(data, dict):
            print("ERROR! Data must be provided as a dictionary.")
            return False
        if not data:
            print("ERROR! Empty data cannot be inserted.")
            return False
        try:
            result = self.collection.insert_one(data)
            return result.acknowledged
        except errors.PyMongoError as e:
            print(f"ERROR! Insert failed: {e}")
            return False

    # READ QUERY
    def read(self, query: dict = None) -> list:
        """Retrieve documents based on a query dictionary."""
        try:
            cursor = self.collection.find(query or {}, {"_id": False})
            return list(cursor)
        except errors.PyMongoError as e:
            print(f"[ERROR] Read failed: {e}")
            return []

    # UPDATE QUERY
    def update(self, query: dict, new_values: dict) -> int:
        """Update documents matching the query with new values."""
        if not query or not new_values:
            print("[ERROR] Both query and new_values must be provided.")
            return 0
        try:
            result = self.collection.update_many(query, {"$set": new_values})
            print(f"[INFO] {result.modified_count} document(s) updated.")
            return result.modified_count
        except errors.PyMongoError as e:
            print(f"[ERROR] Update failed: {e}")
            return 0

    # DELETE QUERY
    def delete(self, query: dict) -> int:
        """Delete documents matching the query."""
        if not query:
            print("[ERROR] Delete query cannot be empty.")
            return 0
        try:
            result = self.collection.delete_many(query)
            print(f"[INFO] {result.deleted_count} document(s) deleted.")
            return result.deleted_count
        except errors.PyMongoError as e:
            print(f"[ERROR] Delete failed: {e}")
            return 0

    # AGGREGATE DATA
    def get_avg_stay_by_breed(self) -> list:
        """Return average stay duration grouped by breed."""
        try:
            pipeline = [
                {"$match": {"breed": {"$exists": True}, "age_upon_outcome_in_weeks": {"$exists": True}}},
                {"$group": {
                    "_id": "$breed",
                    "average_stay_weeks": {"$avg": "$age_upon_outcome_in_weeks"},
                    "count": {"$sum": 1}
                }},
                {"$sort": {"average_stay_weeks": -1}}
            ]
            results = list(self.collection.aggregate(pipeline))
            return results
        except errors.PyMongoError as e:
            print(f"[ERROR] Aggregation failed: {e}")
            return []

# Example usage (optional quick check)
if __name__ == "__main__":
    shelter = AnimalShelter()
    print(shelter.read())  # prints an empty list until data is inserted