#!/bin/bash

# Zoom Bot Environment Setup Script
# This script helps you configure environment variables for the Zoom Bot

set -e

echo "🚀 Zoom Bot Environment Setup"
echo "=============================="
echo ""

# Check if .env.local already exists
if [ -f ".env.local" ]; then
    echo "⚠️  .env.local already exists."
    read -p "Do you want to overwrite it? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Setup cancelled."
        exit 0
    fi
fi

# Copy example file
if [ ! -f ".env.example" ]; then
    echo "❌ .env.example file not found. Please make sure you're in the project root directory."
    exit 1
fi

cp .env.example .env.local
echo "✅ Created .env.local from template"

echo ""
echo "📝 Now you need to edit .env.local with your actual credentials:"
echo ""
echo "1. Go to https://marketplace.zoom.us/develop/apps"
echo "2. Create a 'Server-to-Server OAuth' app for OAuth credentials"
echo "3. Create a 'Meeting SDK' app for SDK credentials"
echo "4. Update .env.local with your actual values"
echo ""

# Try to open the file in a text editor
if command -v code &> /dev/null; then
    echo "Opening .env.local in VS Code..."
    code .env.local
elif command -v nano &> /dev/null; then
    echo "Opening .env.local in nano..."
    nano .env.local
elif command -v vi &> /dev/null; then
    echo "Opening .env.local in vi..."
    vi .env.local
else
    echo "Please edit .env.local manually with your preferred text editor:"
    echo "  nano .env.local"
    echo "  vi .env.local"
fi

echo ""
echo "🔧 After editing .env.local, load the environment variables:"
echo "  source .env.local"
echo ""
echo "🏗️  Then build and run the bot:"
echo "  cd build && make"
echo "  ./zoom_poc"
echo ""
echo "📚 Need help getting credentials? Check the README.md file!"