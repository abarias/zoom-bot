#!/bin/bash

# Test script for the console input functionality
# This script demonstrates how to provide meeting details to the zoom_poc binary

echo "🧪 Testing Zoom Bot Console Input Functionality"
echo "=============================================="
echo ""

echo "This test will simulate providing meeting details via console input"
echo "Meeting Number Format: XXX XXXX XXXX (e.g., 123 4567 8901)"
echo ""

# Check if required environment variables are set
if [[ -z "$ZOOM_CLIENT_ID" || -z "$ZOOM_CLIENT_SECRET" || -z "$ZOOM_ACCOUNT_ID" || -z "$ZOOM_APP_KEY" || -z "$ZOOM_APP_SECRET" ]]; then
    echo "❌ Missing required environment variables for Zoom credentials."
    echo ""
    echo "Please set these environment variables:"
    echo "  export ZOOM_CLIENT_ID=your_client_id"
    echo "  export ZOOM_CLIENT_SECRET=your_client_secret"
    echo "  export ZOOM_ACCOUNT_ID=your_account_id"
    echo "  export ZOOM_APP_KEY=your_app_key"
    echo "  export ZOOM_APP_SECRET=your_app_secret"
    echo ""
    echo "Note: ZOOM_MEETING_NUMBER and ZOOM_MEETING_PASSWORD are no longer needed!"
    echo "The bot will now ask for them interactively."
    exit 1
fi

echo "✅ Zoom credentials are set in environment variables"
echo ""
echo "Ready to test! The zoom_poc binary will now:"
echo "1. Load Zoom credentials from environment variables"
echo "2. Prompt you for meeting number (format: XXX XXXX XXXX)"
echo "3. Prompt you for meeting password"
echo "4. Attempt to join the meeting with streaming enabled"
echo ""
echo "Starting zoom_poc..."
echo "==================="

cd "$(dirname "$0")/build"
./zoom_poc