# 🔧 C++ Debugging Troubleshooting Guide

## ✅ **Current Setup Status:**
- **GDB**: Installed at `/usr/bin/gdb` (version 12.1) ✓
- **Debug Symbols**: Present in binaries ✓ 
- **VS Code C++ Extension**: Installed ✓
- **CMake Tools**: Installed ✓
- **Build Configuration**: Debug mode with `-g -O0 -DDEBUG` ✓

## 🚀 **Quick Debugging Test:**

### **Step 1: Test Simple Program**
1. Select **"Debug Simple Test"** from the debug dropdown
2. Press **F5** - should stop at the start of main()
3. Press **F10** to step through the simple program

### **Step 2: Test Zoom Bot**
1. Select **"Debug Zoom Bot (Simple)"** from the dropdown  
2. Press **F5** - should launch the main application
3. Set a breakpoint in `main()` at line ~389

## 🐛 **Troubleshooting Common Issues:**

### **"Unable to start debugging. The value of miDebuggerPath is invalid"**

**Solution Options:**

1. **Try the Simple Configuration First:**
   - Use **"Debug Zoom Bot (Simple)"** instead of the advanced one
   - This has minimal settings and should work

2. **Verify GDB Installation:**
   ```bash
   which gdb  # Should return: /usr/bin/gdb
   gdb --version  # Should show version info
   ```

3. **Manual GDB Test:**
   ```bash
   cd build
   gdb ./zoom_poc
   (gdb) break main
   (gdb) run  # Test manual debugging
   ```

4. **Check File Permissions:**
   ```bash
   ls -la /usr/bin/gdb  # Should be executable
   ls -la build/zoom_poc  # Should be executable 
   ```

### **"No such file or directory" Error**

**Solution:**
```bash
# Rebuild with debug info
cd /workspaces/zoom-bot
rm -rf build && mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### **"Cannot find or open the shared library"**

**Solution - Library Path Issue:**
1. Check if Zoom SDK libraries exist:
   ```bash
   ls -la /usr/local/zoom-sdk/lib*.so
   ```

2. Use the simplified environment in launch config:
   ```json
   "environment": [
       {
           "name": "LD_LIBRARY_PATH",
           "value": "/usr/local/zoom-sdk"
       }
   ]
   ```

### **Debugger Starts but No Symbols**

**Solutions:**
1. **Verify Debug Symbols:**
   ```bash
   objdump -h build/zoom_poc | grep debug
   # Should show .debug_info, .debug_str, etc.
   ```

2. **Check Compile Flags:**
   ```bash
   cd build
   make VERBOSE=1  # Should show -g flag
   ```

3. **Try Alternative Configuration:**
   - Use **"Debug Zoom Bot (Advanced)"** with preLaunchTask

## 📋 **Available Debug Configurations:**

### **1. Debug Zoom Bot (Simple)** ⭐ *Recommended First*
- Minimal configuration 
- No pre-launch tasks
- Basic environment setup
- **Use this if having issues**

### **2. Debug Zoom Bot (Advanced)**
- Full features with build automation
- Pre-launch task to rebuild
- Enhanced logging
- **Use once simple version works**

### **3. Debug Test Auth**
- Lighter testing utility
- Good for OAuth/JWT debugging
- Faster startup

### **4. Debug Simple Test** 🧪 *For Testing*
- Basic C++ program
- Tests that debugging works at all
- **Start here if nothing works**

### **5. Attach to Running Process**
- For live debugging
- Requires process ID
- Advanced usage

## 🔍 **Diagnostic Commands:**

### **Check Debug Setup:**
```bash
# Verify all tools are present
which gdb cmake g++

# Check binary has debug symbols  
file build/zoom_poc | grep "not stripped"
objdump -h build/zoom_poc | grep -c debug

# Test GDB can read symbols
gdb build/zoom_poc -ex "info functions main" -ex "quit"
```

### **Check VS Code Extensions:**
- Press **Ctrl+Shift+P** → **"Extensions: Show Installed Extensions"**
- Verify **"C/C++"** by Microsoft is installed and enabled

### **Manual Debugging Test:**
```bash
# Run a complete debugging session manually
cd build
gdb ./zoom_poc
(gdb) break main
(gdb) run
(gdb) info locals
(gdb) next  # Step through a few lines
(gdb) quit
```

## ⚡ **Quick Fixes:**

### **If VS Code Debug Panel Shows No Configurations:**
1. **Ctrl+Shift+P** → **"Debug: Select and Start Debugging"**
2. Choose **"C++ (GDB/LLDB)"** 
3. Select **"g++ - Build and debug active file"**

### **If Nothing Works:**
1. Try the **"Debug Simple Test"** configuration first
2. If that fails, reinstall the C++ extension:
   - **Ctrl+Shift+P** → **"Extensions: Reload Window"**
3. Use manual GDB debugging as fallback

### **Performance Issues:**
- Use **"Debug Zoom Bot (Simple)"** for faster startup
- Skip pre-launch tasks if build is current
- Disable advanced logging options

## 🎯 **Next Steps Once Working:**

1. **Set breakpoints** in key functions:
   - `main()` - line ~389
   - `getMeetingDetailsFromConsole()` - line ~340  
   - `AudioRawHandler::onOneAudioDataReceived()`

2. **Use debug console** to inspect variables:
   - Type variable names to see values
   - Use `-exec` commands for GDB features

3. **Step through code** with F10/F11 to understand flow

**Happy debugging! 🐛➡️🎯**