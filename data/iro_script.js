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
        id: 'hue-slider',
        sliderType: 'red'
      }
    },
    {
      component: iro.ui.Slider,
      options: {
        id: 'hue-slider',
        sliderType: 'green'
      }
    },
    {
      component: iro.ui.Slider,
      options: {
        id: 'hue-slider',
        sliderType: 'blue'
      }
    },
    {
      component: iro.ui.Slider,
      options: {
        id: 'hue-slider',
        sliderType: 'value'
      }
    },
    {
      component: iro.ui.Slider,
      options: {
        id: 'hue-slider',
        sliderType: 'kelvin'
      }
    }
  ]
});

colorPicker.on('color:change', function(color) {
  console.log("New color selected:", color.rgbString);
  fetch("/postRGB", {
    method: "POST",
    body: "r=" + color.rgb.r + "&g=" + color.rgb.g + "&b=" + color.rgb.b
  })
  .then(response => console.log(response))
  .catch(error => console.log(error));
});