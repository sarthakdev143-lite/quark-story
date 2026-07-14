"""
Flask Application Factory Module

Creates and configures the Flask application instance with all
necessary extensions, blueprints, and middleware.
"""
import logging
import sys
from typing import Optional

from flask import Flask
from flask_cors import CORS

from .config import Config
from .routes import api_bp


def setup_logging(app: Flask) -> None:
    """
    Configure application logging.
    
    Args:
        app: Flask application instance
    """
    # Set up logging format
    formatter = logging.Formatter(Config.LOG_FORMAT)
    
    # Console handler
    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setFormatter(formatter)
    console_handler.setLevel(getattr(logging, Config.LOG_LEVEL))
    
    # Add handlers to app logger
    app.logger.addHandler(console_handler)
    app.logger.setLevel(getattr(logging, Config.LOG_LEVEL))
    app.logger.info(f"Logging configured at {Config.LOG_LEVEL} level")


def create_app(config_object: Optional[object] = None) -> Flask:
    """
    Application factory function.
    
    Creates and configures a Flask application instance with all
    necessary extensions, blueprints, and middleware.
    
    Args:
        config_object: Optional configuration object to override defaults.
                      If None, uses Config class defaults.
    
    Returns:
        Configured Flask application instance
    """
    app = Flask(__name__)
    
    # Load configuration
    if config_object:
        app.config.from_object(config_object)
    else:
        app.config.from_object(Config)
    
    # Configure logging
    setup_logging(app)
    
    # Initialize CORS with configurable origins
    cors_origins = app.config.get('CORS_ORIGINS', ['*'])
    if isinstance(cors_origins, str) and cors_origins != '*':
        cors_origins = [origin.strip() for origin in cors_origins.split(',')]
    
    CORS(app, resources={r"/api/*": {"origins": cors_origins}})
    app.logger.info(f"CORS enabled for origins: {cors_origins}")
    
    # Register blueprints
    app.register_blueprint(api_bp)
    app.logger.info("API blueprint registered")
    
    # Root endpoint for basic health check
    @app.route('/')
    def index():
        """Root endpoint with API information."""
        return {
            'service': 'Story API',
            'version': '1.0.0',
            'endpoints': {
                'health': '/api/v1/health',
                'stories': '/api/v1/stories',
                'story_detail': '/api/v1/stories/<id>',
                'count': '/api/v1/stories/count'
            }
        }
    
    # Error handlers
    @app.errorhandler(404)
    def not_found(error):
        """Handle 404 errors."""
        return {'success': False, 'error': 'Resource not found'}, 404
    
    @app.errorhandler(500)
    def internal_error(error):
        """Handle 500 errors."""
        app.logger.error(f"Internal server error: {error}")
        return {'success': False, 'error': 'Internal server error'}, 500
    
    app.logger.info("Application factory completed successfully")
    
    return app