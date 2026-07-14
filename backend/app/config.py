import os
from typing import TypedDict


class StoryData(TypedDict):
    """Type definition for story data structure."""
    id: int
    title: str
    author: str
    genre: str
    summary: str
    content: str
    publish_date: str
    read_time: str


class Config:
    """Application configuration with environment variable support."""
    
    # Base directory for file paths
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    
    # Data file location
    DATA_FILE = os.path.join(BASE_DIR, '..', 'data', 'stories.json')
    
    # API Configuration
    API_PREFIX = '/api'
    API_VERSION = 'v1'
    
    # Server Configuration
    HOST = os.getenv('API_HOST', '0.0.0.0')
    PORT = int(os.getenv('API_PORT', 5000))
    DEBUG = os.getenv('DEBUG', 'False').lower() in ('true', '1', 'yes')
    
    # CORS Configuration
    CORS_ORIGINS = os.getenv('CORS_ORIGINS', '*').split(',')
    
    # Logging Configuration
    LOG_LEVEL = os.getenv('LOG_LEVEL', 'INFO')
    LOG_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'