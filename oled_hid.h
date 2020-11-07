#include QMK_KEYBOARD_H
#include "remote_kb.h" 
#include <string.h>
#include "raw_hid.h"

// Define which oled screens you want to see
#define show_stocks false
#define show_weather true
#define show_performance true

// Define which oled screen to start on
// 0: stocks   1: weather   2: performance
int volatile current_screen = 2;

// screen options: [stocks, weather, performance]
bool screen_options[3] = {show_stocks, show_weather, show_performance};
uint8_t num_screens = 3;
bool volatile host_connected = false;

// Use this to convert from unicode to a letter
const char unicode_alpha[256] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void stock_data_to_oled(int * stonk) {

  oled_clear();
  oled_render();

  // Write the stock ticker
  oled_set_cursor(0, 0);
  static char l1[28] = {0};
  snprintf(l1, sizeof(l1), "%c%c%c%c%c\n", unicode_alpha[stonk[5]-64], unicode_alpha[stonk[6]-64], unicode_alpha[stonk[7]-64], unicode_alpha[stonk[8]-64], unicode_alpha[stonk[9]-64]);
  oled_write(l1, false);

  // Write the percent change from beginning of trading
  if (stonk[4] > 0) {
    oled_set_cursor(0, 1);
  } else {
    oled_set_cursor(0, 2);
  }
  
  static char l2[28] = {0};
  snprintf(l2, sizeof(l2), "%d%%\n", stonk[0]-128);
  oled_write(l2, false);
  
  // Write the current price
  oled_set_cursor(0, 3);
  static char l4[28] = {0};
  if (stonk[3] > 10) {
    snprintf(l4, sizeof(l4), "$%d.%d\n", (stonk[1] << 8) + stonk[2], stonk[3]);
  } else {
    snprintf(l4, sizeof(l4), "$%d.0%d\n", (stonk[1] << 8) + stonk[2], stonk[3]);
  }
  oled_write(l4, false);


  // Write the (x, y) coordinates to display the graph
  for (int i = 12; i < 390; i += 2) {
    if (stonk[i] > 0 && stonk[i+1] > 0) {
      oled_write_pixel(stonk[i], stonk[i+1], true);
    }
  }
  
}

void weather_data_to_oled(int * data) {
  int current_id = data[0] + (data[1] << 8);
  uint8_t current_temp = data[2];
  uint8_t todays_min = data[3];
  uint8_t todays_max = data[4];
  uint8_t is_night = data[6];

  uint8_t next_update_time = data[10];
  uint8_t temp_at_next_update = data[11];
  int id_at_next_update = data[12] + (data[13] << 8);
  uint8_t chance_of_precip = data[14];

  char cur_conditions[8];
  char next_conditions[8];

  oled_clear();
  oled_render();

  // The purpose of this for loop is to draw the icon corresponding to the current
  // weather conditions. This works by sending over a series of bytes in groups of
  // three. The first two bytes are used to determine the index of the oled that
  // should be drawn into. The third byte is used to determine which pixels to
  // illuminate.
  int pos;
  for (int i = 20; i < 390; i += 3) {
    if (data[i] > 0 || data[i+1] > 0) {
      pos = (data[i] << 8) + data[i+1];
      oled_write_raw_byte(data[i+2], pos);
    }
  }

  // weather condition codes: https://openweathermap.org/weather-conditions
  if (current_id < 300) {
    strcpy(cur_conditions, "Storms");
  } else if (current_id == 800) {
    strcpy(cur_conditions, "Clear");
  } else if (current_id < 400) {
    strcpy(cur_conditions, "Drizzle");
  } else if (current_id < 600) {
    strcpy(cur_conditions, "Rain");
  } else if (current_id < 700) {
    strcpy(cur_conditions, "Snow");
  } else if (current_id < 800) {
    strcpy(cur_conditions, "Fog");
  } else if (current_id > 800) {
    if (current_id == 804) {  
      strcpy(cur_conditions, "Cloudy");
    } else if (is_night) {  
      strcpy(cur_conditions, "P.Cloudy");
    } else {  
      strcpy(cur_conditions, "P.Cloudy");
    }
  }


  if (id_at_next_update < 300) {
    strcpy(next_conditions, "Storms");
  } else if (id_at_next_update == 800) {
    strcpy(next_conditions, "Clear");
  } else if (id_at_next_update < 400) {
    strcpy(next_conditions, "Drizz");
  } else if (id_at_next_update < 600) {
    strcpy(next_conditions, "Rain");
  } else if (id_at_next_update < 700) {
    strcpy(next_conditions, "Snow");
  } else if (id_at_next_update < 800) {
    strcpy(next_conditions, "Fog");
  } else if (id_at_next_update > 800) {
    strcpy(next_conditions, "Cloudy");
  }

  // Todays high temperature
  oled_set_cursor(0, 0);
  static char l1l[7] = {0};
  snprintf(l1l, sizeof(l1l), "H: %d", todays_max);
  oled_write(l1l, false);

  // Current temperature
  oled_set_cursor(0, 1);
  static char l2l[9] = {0};
  snprintf(l2l, sizeof(l2l), "Now: %d", current_temp);
  oled_write(l2l, false);

  // Current weather conditions (as text)
  oled_set_cursor(0, 2);
  oled_write(cur_conditions, false);

  // Todays low temp
  oled_set_cursor(0, 3);
  static char l4l[7] = {0};
  snprintf(l4l, sizeof(l4l), "L: %d", todays_min);
  oled_write(l4l, false);

  // Time of next update
  oled_set_cursor(15, 0);
  static char l1r[7] = {0};
  snprintf(l1r, sizeof(l1r), "%d:00", next_update_time);
  oled_write(l1r, false);

  // Temperature at next update
  oled_set_cursor(15, 1);
  static char l2r[7] = {0};
  snprintf(l2r, sizeof(l2r), "%d F", temp_at_next_update);
  oled_write(l2r, false);

  // Chance of precipitation
  oled_set_cursor(15, 2);
  static char l3r[8] = {0};
  snprintf(l3r, sizeof(l3r), "P: %d%%", chance_of_precip);
  oled_write(l3r, false);

  // Weather conditions (as text) at next update
  oled_set_cursor(15, 3);
  static char l4r[7] = {0};
  snprintf(l4r, sizeof(l4r), "%s", next_conditions);
  oled_write(l4r, false);

}


void performance_data_to_oled(int * data) {
  oled_write_ln_P(PSTR("VOL:"), false);
  oled_write_ln_P(PSTR("RAM:"), false);
  oled_write_ln_P(PSTR("CPU:"), false);
  oled_write_ln_P(PSTR("SSD:"), false);

  int cur_row;
  for (uint8_t stat = 0; stat < 4; stat++) {
    cur_row = stat * 8 + 1;
    for (uint8_t i = 0; i < 6; i++) {
      for (int j = 25; j < data[stat]; j++) {
        oled_write_pixel(j, cur_row+i, true);
      }
    }
  }
}

int arr[400]; // holds data sent from host pc
int arr_pt = 0; // stores current location in arr since we can only recieve 32 bytes at a time
uint8_t prev_screen = 0;
void raw_hid_receive(uint8_t *data, uint8_t length) {

  // Check if this is a new incoming connection
  if (data[0] == 127) {
    arr_pt = 0;
    host_connected = true;
    return;
  }

  // Add data from host pc to our array
  for (uint8_t i = 1; i < 31; i++) {
    arr[arr_pt] = data[i];
    arr_pt++;
  }

  // Check if we've received all of the data
  if (arr_pt >= 390) {

    // Clear the screen before rendering something new
    if (prev_screen != current_screen) {
      oled_clear();
      oled_render();
    }
    prev_screen = current_screen;

    if (current_screen == 1){
      stock_data_to_oled(arr);
    } else if (current_screen == 2) {
      weather_data_to_oled(arr);
    } else {
      performance_data_to_oled(arr);
    }
    arr_pt = 0; // reset arr_pt
  }
}

void update_oled(void) {
    current_screen++;
    if (current_screen > num_screens) {
        current_screen = 1;
    }
    int i = 0;
    while (i < 3) {
        if (screen_options[current_screen-1]) {
            break;
        } else {
            current_screen++;
        }
        i++;
    }
    uint8_t send_data[32] = {0};
    send_data[0] = current_screen;
    raw_hid_send(send_data, sizeof(send_data)); // let the host know we've switched
                                                // screens -- send new data to kb. 
}
