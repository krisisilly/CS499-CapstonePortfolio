from pymongo import MongoClient
from bson.objectid import ObjectId

class AnimalShelter(object):

    def __init__(self, username, password, host, port, db='AAC', collection='animals'):

        try:
            self.client = MongoClient(f"mongodb://{username}:{password}@{host}:{port}/?authSource=admin")
            self.database = self.client[db]
            self.collection = self.database[collection]
        except Exception as e:
            print(f/"Failed connection: {e}")
            raise

    def create(self, data):

        if data is None:
            raise ValueError("Data is empty")
        try:
            result = self.collection.insert_one(data)
            return result.acknowledged
        except Exception as e:
            print(f"Failed to create: {e}")
            return False

    def read(self, query):

        if query is None:
            raise ValueError("Query is empty")
        try:
            documents = list(self.collection.find(query))
            return documents
        except Exception as e:
            print(f"Failed to read: {e}")
            return []

    def update(self, query, new_data):

        if query is None or new_data is None:
            raise ValueError("Need query and new data")
        try:
            result = self.collection.update_many(query, {"$set": new_data})
            return result.modified_count
        except Exception as e:
            print(f"Update failed: {e}")
            return 0

    def delete(self, query):

        if query is None:
            raise ValueError("Provide query")
        try:
            result = self.collection.delete_many(query)
            return result.deleted_count
        except Exception as e:
            print(f"Delete failed: {e}")
            return 0

