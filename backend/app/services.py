"""
Story Service Module

Provides business logic for story data access and manipulation.
Follows separation of concerns pattern with routes layer.
"""
import json
import logging
from typing import Optional, List, Dict, Any
from pathlib import Path

from .config import Config, StoryData


logger = logging.getLogger(__name__)


class StoryServiceError(Exception):
    """Custom exception for StoryService errors."""
    pass


class StoryService:
    """
    Service class for managing story data operations.
    
    This class handles loading, caching, and retrieving story data
    from the JSON data source. It provides methods for fetching
    both story summaries and full story details.
    """
    
    def __init__(self, data_file: Optional[str] = None):
        """
        Initialize the StoryService.
        
        Args:
            data_file: Optional path to the stories JSON file.
                      Defaults to Config.DATA_FILE if not provided.
        
        Raises:
            StoryServiceError: If the data file cannot be loaded.
        """
        self._data_file = data_file or Config.DATA_FILE
        self._data: List[Dict[str, Any]] = []
        self._load_data()
    
    def _load_data(self) -> None:
        """
        Load story data from the JSON file.
        
        Raises:
            StoryServiceError: If the file cannot be found or parsed.
        """
        try:
            data_path = Path(self._data_file).resolve()
            if not data_path.exists():
                raise FileNotFoundError(f"Data file not found: {data_path}")
            
            with open(data_path, 'r', encoding='utf-8') as f:
                raw_data = json.load(f)
                self._data = raw_data.get('stories', [])
            
            logger.info(f"Loaded {len(self._data)} stories from {data_path}")
            
        except (json.JSONDecodeError, IOError) as e:
            error_msg = f"Failed to load story data: {e}"
            logger.error(error_msg)
            raise StoryServiceError(error_msg) from e
    
    def get_all_summaries(self) -> List[Dict[str, Any]]:
        """
        Retrieve summary information for all stories.
        
        Returns a list of story summaries excluding the full content
        to optimize network transfer for list views.
        
        Returns:
            List of story summary dictionaries containing:
            - id, title, author, genre, summary, publish_date, read_time
        """
        return [
            {
                'id': story['id'],
                'title': story['title'],
                'author': story['author'],
                'genre': story['genre'],
                'summary': story['summary'],
                'publish_date': story['publish_date'],
                'read_time': story['read_time']
            }
            for story in self._data
        ]
    
    def get_by_id(self, story_id: int) -> Optional[Dict[str, Any]]:
        """
        Retrieve full story details by ID.
        
        Args:
            story_id: The unique identifier of the story to retrieve.
        
        Returns:
            Complete story dictionary including content, or None if not found.
        """
        story = next((s for s in self._data if s['id'] == story_id), None)
        
        if story is None:
            logger.warning(f"Story with ID {story_id} not found")
        else:
            logger.debug(f"Retrieved story '{story['title']}' (ID: {story_id})")
        
        return story
    
    def search_by_genre(self, genre: str) -> List[Dict[str, Any]]:
        """
        Search stories by genre (case-insensitive).
        
        Args:
            genre: The genre to search for.
        
        Returns:
            List of matching story summaries.
        """
        genre_lower = genre.lower()
        matches = [
            s for s in self._data 
            if genre_lower in s.get('genre', '').lower()
        ]
        
        logger.info(f"Found {len(matches)} stories matching genre '{genre}'")
        return [
            {
                'id': s['id'],
                'title': s['title'],
                'author': s['author'],
                'genre': s['genre'],
                'summary': s['summary'],
                'publish_date': s['publish_date'],
                'read_time': s['read_time']
            }
            for s in matches
        ]
    
    def get_count(self) -> int:
        """Return the total number of stories available."""
        return len(self._data)