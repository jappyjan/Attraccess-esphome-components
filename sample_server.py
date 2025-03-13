#!/usr/bin/env python3
"""
Sample SSE server script to test the API Resource Status ESPHome component.

This script simulates a server that provides resource status updates via Server-Sent Events (SSE)
using the same event structure as the Attraccess API.

Run with: python3 sample_server.py
Then configure ESPHome to connect to: http://YOUR_IP_ADDRESS:8000
"""

import json
import time
import random
import datetime
from flask import Flask, Response, jsonify
from threading import Thread, Lock

app = Flask(__name__)

# Sample resource data - in a real application, this would be in a database
resources = {
    "12345": {
        "id": 12345,
        "name": "Test Resource",
        "inUse": False,
        "lastUpdated": time.time(),
        "currentUserId": None,
        "startTime": None,
    }
}

resource_lock = Lock()

def format_iso_time(timestamp=None):
    """Format a timestamp as ISO 8601 format (compatible with API)"""
    if timestamp is None:
        timestamp = time.time()
    dt = datetime.datetime.fromtimestamp(timestamp)
    return dt.isoformat()

def generate_sse_events():
    """Generate Server-Sent Events when resource status changes"""
    while True:
        time.sleep(5)
        
        with resource_lock:
            # Randomly change the status of a resource for demonstration
            for resource_id, resource in resources.items():
                # 20% chance of changing status
                if random.random() < 0.2:
                    # Toggle status
                    new_status = not resource["inUse"]
                    now = time.time()
                    
                    # Create event data based on the new status
                    if new_status:  # Resource is now in use
                        # Simulate a user starting to use the resource
                        user_id = random.randint(1000, 9999)
                        resource["inUse"] = True
                        resource["currentUserId"] = user_id
                        resource["startTime"] = now
                        resource["lastUpdated"] = now
                        
                        # Create event in the original format with added inUse field
                        data = {
                            "resourceId": resource["id"],
                            "userId": user_id,
                            "startTime": format_iso_time(now),
                            "inUse": True,
                            "eventType": "resource.usage.started"
                        }
                        
                        event_name = "update"
                    else:  # Resource is now available
                        # Simulate a user ending their usage
                        start_time = resource["startTime"] or (now - 3600)  # Default to 1 hour ago
                        user_id = resource["currentUserId"]
                        
                        resource["inUse"] = False
                        resource["currentUserId"] = None
                        resource["startTime"] = None
                        resource["lastUpdated"] = now
                        
                        # Create event in the original format with added inUse field
                        data = {
                            "resourceId": resource["id"],
                            "userId": user_id,
                            "startTime": format_iso_time(start_time),
                            "endTime": format_iso_time(now),
                            "inUse": False,
                            "eventType": "resource.usage.ended"
                        }
                        
                        event_name = "update"
                    
                    # Return SSE formatted data
                    event_data = f"event: {event_name}\ndata: {json.dumps(data)}\n\n"
                    print(f"Sending update: {data}")
                    yield event_data

@app.route('/api/resources/<resource_id>', methods=['GET'])
def get_resource(resource_id):
    """API endpoint to get the current status of a resource"""
    if resource_id not in resources:
        return jsonify({"error": "Resource not found"}), 404
        
    with resource_lock:
        return jsonify(resources[resource_id])

@app.route('/api/resources/<resource_id>/events', methods=['GET'])
def resource_events(resource_id):
    """SSE endpoint for real-time updates on a specific resource"""
    if resource_id not in resources:
        return jsonify({"error": "Resource not found"}), 404
        
    # Send headers for SSE
    headers = {
        'Content-Type': 'text/event-stream',
        'Cache-Control': 'no-cache',
        'Connection': 'keep-alive'
    }
    
    # Return initial data immediately
    def stream():
        # Send initial state
        with resource_lock:
            resource = resources[resource_id]
            initial_data = {
                "resourceId": resource["id"],
                "inUse": resource["inUse"],
                "timestamp": format_iso_time(resource["lastUpdated"])
            }
            yield f"event: update\ndata: {json.dumps(initial_data)}\n\n"
        
        # Then send all updates
        yield from generate_sse_events()
    
    return Response(stream(), headers=headers)

@app.route('/api/toggle/<resource_id>', methods=['GET'])
def toggle_resource(resource_id):
    """Helper endpoint to manually toggle a resource status (for testing)"""
    if resource_id not in resources:
        return jsonify({"error": "Resource not found"}), 404
        
    with resource_lock:
        resource = resources[resource_id]
        new_status = not resource["inUse"]
        now = time.time()
        
        # Update resource based on new status
        if new_status:  # Resource is now in use
            user_id = random.randint(1000, 9999)
            resource["inUse"] = True
            resource["currentUserId"] = user_id
            resource["startTime"] = now
            resource["lastUpdated"] = now
            
            return jsonify({
                "resourceId": resource["id"],
                "inUse": True,
                "userId": user_id,
                "timestamp": format_iso_time(now),
                "status": "In Use",
                "message": "Resource marked as in use"
            })
        else:  # Resource is now available
            resource["inUse"] = False
            resource["currentUserId"] = None
            resource["startTime"] = None
            resource["lastUpdated"] = now
            
            return jsonify({
                "resourceId": resource["id"],
                "inUse": False,
                "timestamp": format_iso_time(now),
                "status": "Available",
                "message": "Resource marked as available"
            })

if __name__ == '__main__':
    print("Starting SSE sample server on http://127.0.0.1:8000")
    print("Configure your ESPHome component to use: http://YOUR_IP_ADDRESS:8000")
    print("Test toggling resource status at: http://127.0.0.1:8000/api/toggle/12345")
    app.run(host='0.0.0.0', port=8000, threaded=True) 