from google.cloud import storage
import os

def download_public_bucket(bucket_name, destination_dir):
    client = storage.Client.create_anonymous_client()
    bucket = client.bucket(bucket_name)
    blobs = bucket.list_blobs()

    for blob in blobs:
        dest_file_path = os.path.join(destination_dir, blob.name)
        os.makedirs(os.path.dirname(dest_file_path), exist_ok=True)  # Ensure directory exists

        print(f"Downloading {blob.name}...")
        blob.download_to_filename(dest_file_path)


# Usage
bucket_name = 'compass_osdi'
destination_dir = './data/'

download_public_bucket(bucket_name, destination_dir)