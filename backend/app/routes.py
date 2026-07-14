from flask import Blueprint, jsonify
from .services import StoryService

api_bp = Blueprint('api', __name__)
service = StoryService()

@api_bp.route('/health')
def health():
    return jsonify({'status': 'healthy'})

@api_bp.route('/stories')
def get_stories():
    stories = service.get_all_summaries()
    return jsonify({'success': True, 'count': len(stories), 'data': stories})

@api_bp.route('/stories/<int:story_id>')
def get_story(story_id):
    story = service.get_by_id(story_id)
    if not story:
        return jsonify({'success': False, 'error': 'Story not found'}), 404
    return jsonify({'success': True, 'data': story})