"""
Application Entry Point

Production-ready Flask application runner with support for
both development server and WSGI production deployment.
"""
import os
import sys

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from app import create_app
from app.config import Config


def create_application():
    """
    Create and configure the WSGI application.
    
    This function is called by WSGI servers like gunicorn.
    
    Returns:
        Flask application instance
    """
    return create_app()


if __name__ == '__main__':
    # Development server
    app = create_app()
    
    host = os.getenv('API_HOST', Config.HOST)
    port = int(os.getenv('API_PORT', Config.PORT))
    debug = os.getenv('DEBUG', 'False').lower() in ('true', '1', 'yes')
    
    print(f"Starting Story API development server...")
    print(f"  Host: {host}")
    print(f"  Port: {port}")
    print(f"  Debug: {debug}")
    print(f"\nAPI Endpoints:")
    print(f"  Health:   http://{host}:{port}/api/v1/health")
    print(f"  Stories:  http://{host}:{port}/api/v1/stories")
    print(f"  Root:     http://{host}:{port}/")
    print("\nPress Ctrl+C to stop the server.")
    
    app.run(host=host, port=port, debug=debug)