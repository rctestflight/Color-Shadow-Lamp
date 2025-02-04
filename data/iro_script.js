// Wait for DOM to be fully loaded
document.addEventListener('DOMContentLoaded', function () {
  var colorPicker = new iro.ColorPicker("#color-picker", {
    width: 250,
    color: "rgb(255, 255, 255)",
    borderWidth: 1,
    borderColor: "#fff",
    layout: [
      {
        component: iro.ui.Wheel,
      },
      {
        component: iro.ui.Slider,
        options: {
          id: 'red-slider',
          sliderType: 'red'
        }
      },
      {
        component: iro.ui.Slider,
        options: {
          id: 'green-slider',
          sliderType: 'green'
        }
      },
      {
        component: iro.ui.Slider,
        options: {
          id: 'blue-slider',
          sliderType: 'blue'
        }
      },
      {
        component: iro.ui.Slider,
        options: {
          id: 'value-slider',
          sliderType: 'value'
        }
      },
      {
        component: iro.ui.Slider,
        options: {
          id: 'kelvin-slider',
          sliderType: 'kelvin'
        }
      }
    ]
  });

  // Get DOM elements
  const lockStatus = document.getElementById('lockStatus');
  const lockStatusText = document.getElementById('lockStatusText');
  const unlockButton = document.getElementById('unlockButton');
  const resetButton = document.getElementById('resetButton');

  // Only proceed if we have all required elements
  if (!lockStatus || !lockStatusText || !unlockButton || !resetButton) {
    console.error('Required DOM elements not found');
    return;
  }

  // Throttle/debounce utility
  function debounce(func, wait) {
    let timeout;
    let lastArgs;
    let lastTime = 0;
    const throttleWait = 50; // Minimum time between updates (ms)

    return function executedFunction() {
      const args = arguments;
      const now = Date.now();

      // Clear the previous timeout
      if (timeout) {
        clearTimeout(timeout);
      }

      // If enough time has passed, execute immediately
      if (now - lastTime >= throttleWait) {
        func.apply(this, args);
        lastTime = now;
      } else {
        // Otherwise, set up a debounced call
        lastArgs = args;
        timeout = setTimeout(() => {
          func.apply(this, lastArgs);
          lastTime = Date.now();
        }, wait);
      }
    };
  }

  // Debounced update function
  const updateColor = debounce(function (color) {
    fetch("/postRGB", {
      method: "POST",
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
      },
      body: "r=" + color.rgb.r + "&g=" + color.rgb.g + "&b=" + color.rgb.b
    })
      .then(response => {
        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
        console.log('Color updated');
      })
      .catch(error => console.error('Error updating color:', error));
  }, 100); // 100ms debounce time

  // Color change handler
  colorPicker.on('color:change', function (color) {
    updateColor(color);
  });

  function updateLockStatus() {
    fetch('/lockStatus')
      .then(response => {
        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
        return response.json();
      })
      .then(data => {
        console.log('Lock status response:', data); // Debug logging
        const unlocked = data.unlocked;
        lockStatus.className = 'status-indicator ' + (unlocked ? 'status-unlocked' : 'status-locked');
        lockStatusText.textContent = unlocked ? 'FULL POWER MODE' : 'SAFE MODE';
        unlockButton.disabled = unlocked;
        resetButton.style.display = unlocked ? 'inline-block' : 'none';
      })
      .catch(error => {
        console.error('Error checking lock status:', error);
        lockStatusText.textContent = 'Error checking status';
      });
  }

  // Unlock button handler
  unlockButton.addEventListener('click', function () {
    if (confirm('WARNING: Unlocking full power mode will allow the LEDs to operate at higher power levels. ' +
      'This can cause excessive heat and potentially damage the LEDs if run for extended periods. ' +
      'Are you sure you want to continue?')) {
      fetch('/unlock', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        }
      })
        .then(response => {
          if (response.ok) {
            console.log('Unlock successful');
            updateLockStatus();
          } else {
            throw new Error('Unlock request failed');
          }
        })
        .catch(error => {
          console.error('Error unlocking:', error);
          alert('Failed to unlock device');
        });
    }
  });

  // Reset button handler
  resetButton.addEventListener('click', function () {
    if (confirm('This will return the device to safe mode (10% power limit). Continue?')) {
      fetch('/reset', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
        }
      })
        .then(response => {
          if (response.ok) {
            console.log('Reset successful');
            updateLockStatus();
          } else {
            throw new Error('Reset request failed');
          }
        })
        .catch(error => {
          console.error('Error resetting:', error);
          alert('Failed to reset device');
        });
    }
  });

  // Initial status check
  updateLockStatus();
  // Poll every 5 seconds
  setInterval(updateLockStatus, 5000);
});