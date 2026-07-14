"""
API Routes Module

Defines Flask Blueprint for story API endpoints.
Follows RESTful conventions with proper error handling.
"""
import logging
from typing import Tuple, Dict, Any

from flask import Blueprint, jsonify, Response, request

from .services import StoryService, StoryServiceError


logger = logging.getLogger(__name__)

# Create Blueprint with versioned URL prefix
api_bp = Blueprint('api', __name__, url_prefix='/api/v1')

# Initialize service as singleton
_story_service: StoryService = None


def get_service() -> StoryService:
    """Lazy initialization of StoryService singleton."""
    global _story_service
    if _story_service is None:
        _story_service = StoryService()
    return _story_service


@api_bp.route('/health', methods=['GET'])
def health_check() -> Response:
    """
    Health check endpoint for monitoring and load balancer probes.
    
    Returns:
        JSON response with service status.
    """
    logger.debug("Health check requested")
    return jsonify({
        'status': 'healthy',
        'service': 'story-api',
        'version': '1.0.0'
    }), 200


@api_bp.route('/stories', methods=['GET'])
def get_stories() -> Response:
    """
    Retrieve all story summaries.
    
    Query Parameters:
        genre (optional): Filter stories by genre (case-insensitive partial match)
    
    Returns:
        JSON response containing:
        - success: boolean indicating request success
        - count: number of stories returned
        - data: array of story summary objects
    """
    try:
        service = get_service()
        
        # Check for optional genre filter
        genre = request.args.get('genre')
        if genre:
            logger.info(f"Filtering stories by genre: {genre}")
            stories = service.search_by_genre(genre)
        else:
            stories = service.get_all_summaries()
        
        logger.info(f"Returning {len(stories)} story summaries")
        
        return jsonify({
            'success': True,
            'count': len(stories),
            'data': stories
        }), 200
        
    except StoryServiceError as e:
        logger.error(f"Service error while fetching stories: {e}")
        return jsonify({
            'success': False,
            'error': 'Failed to retrieve stories',
            'details': str(e)
        }), 500


@api_bp.route('/stories/<int:story_id>', methods=['GET'])
def get_story(story_id: int) -> Tuple[Response, int]:
    """
    Retrieve full details for a specific story.
    
    Path Parameters:
        story_id: The unique identifier of the story
    
    Returns:
        JSON response containing:
        - success: boolean indicating request success
        - data: complete story object (if found)
        - error: error message (if not found or error occurred)
    """
    try:
        service = get_service()
        story = service.get_by_id(story_id)
        
        if story is None:
            logger.warning(f"Story not found: ID={story_id}")
            return jsonify({
                'success': False,
                'error': 'Story not found',
                'data': None
            }), 404
        
        logger.info(f"Returning story details: ID={story_id}, Title='{story['title']}'")
        
        return jsonify({
            'success': True,
            'data': story
        }), 200
        
    except StoryServiceError as e:
        logger.error(f"Service error while fetching story {story_id}: {e}")
        return jsonify({
            'success': False,
            'error': 'Failed to retrieve story details',
            'details': str(e)
        }), 500


@api_bp.route('/stories/count', methods=['GET'])
def get_story_count() -> Response:
    """
    Get total count of available stories.
    
    Returns:
        JSON response with story count.
    """
    try:
        service = get_service()
        count = service.get_count()
        
        return jsonify({
            'success': True,
            'count': count
        }), 200
        
    except StoryServiceError as e:
        logger.error(f"Service error while getting story count: {e}")
        return jsonify({
            'success': False,
            'error': 'Failed to get story count',
            'details': str(e)
        }), 500


# No circular import needed - request is imported at module level