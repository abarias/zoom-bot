#!/bin/bash

echo "=== Recording Permission Fix Test ==="
echo ""
echo "This test will:"
echo "1. Join a meeting" 
echo "2. Request recording permission from host"
echo "3. Wait for host approval"
echo "4. Retry audio setup after permission is granted"
echo ""
echo "Instructions:"
echo "- Start the bot"
echo "- Join a meeting with waiting room/recording permissions"
echo "- When host approves recording, bot should automatically start recording"
echo ""
echo "Starting bot..."

# Set up audio first
/workspaces/zoom-bot/setup-simple-audio.sh

# Run the bot
/workspaces/zoom-bot/run-zoom-bot.sh