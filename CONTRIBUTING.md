# Contributing to Zoom Bot

Thank you for considering contributing to the Zoom Bot project! Here are some guidelines to help you get started.

## 🚀 Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR-USERNAME/zoom-bot.git
   cd zoom-bot
   ```
3. **Create a new branch** for your feature:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## 💻 Development Setup

1. **Install dependencies**:
   ```bash
   sudo apt update
   sudo apt install build-essential cmake libglib2.0-dev nlohmann-json3-dev
   ```

2. **Build the project**:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```

3. **Run tests**:
   ```bash
   ./test_auth  # Test authentication
   ./zoom_poc   # Test main functionality
   ```

## 📝 Code Guidelines

### Code Style
- Use **4 spaces** for indentation (no tabs)
- Follow **C++17** standards
- Use **camelCase** for variables and functions
- Use **PascalCase** for classes and namespaces
- Add comprehensive **comments** for complex logic

### File Organization
- Header files (`.h`) in same directory as implementation (`.cpp`)
- Keep classes focused and single-responsibility
- Use forward declarations where possible
- Include guards or `#pragma once` in headers

### Example Code Style:
```cpp
namespace ZoomBot {

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();
    
    bool initialize();
    void processAudioData(const AudioData* data);

private:
    std::string outputDirectory_;
    std::mutex processingMutex_;
    
    bool validateAudioData(const AudioData* data);
    void writeToFile(const std::string& filename, const char* data, size_t length);
};

} // namespace ZoomBot
```

## 🧪 Testing

### Manual Testing
1. **Join a test meeting** with the bot
2. **Verify permission flow** works correctly
3. **Check audio file output** quality and format
4. **Test graceful shutdown** (Ctrl+C)

### Adding Tests
- Add unit tests for new functionality
- Test error conditions and edge cases
- Verify memory management (no leaks)
- Test with different meeting configurations

## 🐛 Bug Reports

When reporting bugs, please include:

### Required Information
- **Operating System** and version
- **Compiler version** (`gcc --version`)
- **CMake version** (`cmake --version`)
- **Steps to reproduce** the issue
- **Expected vs actual behavior**
- **Log output** (if available)

### Bug Report Template
```markdown
**Environment:**
- OS: Ubuntu 22.04
- GCC: 11.4.0
- CMake: 3.22.1

**Steps to Reproduce:**
1. Start the bot with `./zoom_poc`
2. Join meeting ID 123456789
3. Host denies recording permission
4. [Describe what happens]

**Expected Behavior:**
Bot should respect denial and not attempt recording

**Actual Behavior:**
Bot continues trying to record audio

**Logs:**
[Paste relevant log output here]
```

## 💡 Feature Requests

### Good Feature Requests Include:
- **Clear description** of the problem being solved
- **Specific use cases** that would benefit
- **Proposed implementation approach** (if you have ideas)
- **Consideration of impact** on existing functionality

### Feature Areas We're Interested In:
- Multi-meeting support
- Real-time audio processing
- Cloud storage integration
- Improved error handling
- Performance optimizations
- Security enhancements

## 🔄 Pull Request Process

### Before Submitting
1. **Ensure code builds** without warnings
2. **Test functionality** manually
3. **Update documentation** if needed
4. **Add comments** explaining complex logic
5. **Check for memory leaks** with valgrind (if possible)

### PR Description Template
```markdown
## Description
Brief description of what this PR does.

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Manual testing performed
- [ ] Added unit tests (if applicable)
- [ ] Tested with real Zoom meetings
- [ ] Memory leak testing

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] No console warnings or errors
```

### PR Review Process
1. **Automated checks** run (if configured)
2. **Manual review** by maintainers
3. **Testing** by reviewers
4. **Discussion** and feedback
5. **Merge** after approval

## 🏷️ Commit Guidelines

### Commit Message Format
```
type(scope): short description

Longer description if needed.

- List any breaking changes
- Reference issues: Fixes #123
```

### Commit Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

### Examples
```bash
feat(audio): add real-time audio processing support

- Implemented streaming audio processor
- Added configurable filter chains
- Supports multiple output formats

Fixes #45
```

## 🆘 Getting Help

### Where to Ask Questions
- **GitHub Issues**: For bugs and feature requests
- **Discussions**: For general questions and ideas
- **Documentation**: Check existing docs first

### What to Include in Questions
- What you're trying to accomplish
- What you've already tried
- Specific error messages
- Relevant code snippets

## 📚 Resources

### Zoom SDK Documentation
- [Zoom Meeting SDK](https://developers.zoom.us/docs/meeting-sdk/)
- [Raw Data API](https://developers.zoom.us/docs/meeting-sdk/linux/rawdata/)

### C++ Resources
- [C++ Reference](https://en.cppreference.com/)
- [Modern C++ Guidelines](https://isocpp.github.io/CppCoreGuidelines/)

### Audio Processing
- [PCM Format Specification](https://en.wikipedia.org/wiki/Pulse-code_modulation)
- [WAV File Format](http://soundfile.sapp.org/doc/WaveFormat/)

Thank you for contributing to make Zoom Bot better! 🎉