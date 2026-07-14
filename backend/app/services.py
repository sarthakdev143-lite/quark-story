import json
from .config import Config

class StoryService:
    def __init__(self):
        with open(Config.DATA_FILE, 'r') as f:
            self._data = json.load(f)['stories']

    def get_all_summaries(self):
        return [
            {
                'id': s['id'],
                'title': s['title'],
                'author': s['author'],
                'genre': s['genre'],
                'summary': s['summary'],
                'publish_date': s['publish_date'],
                'read_time': s['read_time']
            } for s in self._data
        ]

    def get_by_id(self, story_id):
        return next((s for s in self._data if s['id'] == story_id), None)