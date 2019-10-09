<template>
  <v-app>
   <v-container>
   <v-row>

   <v-col justify-self="end">
   <v-card class="mx-auto" min-width="350">
    <v-container>
      <v-row>
        <v-col>
          <v-card><v-btn tile id="color-tile-0" v-bind:color="colors[0]" v-on:click="cb_action(0)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile id="color-tile-1" v-bind:color="colors[1]" v-on:click="cb_action(1)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile id="color-tile-2" v-bind:color="colors[2]" v-on:click="cb_action(2)"></v-btn></v-card>
        </v-col>
      </v-row>
      <v-row>
        <v-col>
          <v-card><v-btn tile id="color-tile-3" v-bind:color="colors[3]" v-on:click="cb_action(3)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile id="color-tile-4" v-bind:color="colors[4]" v-on:click="cb_action(4)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile id="color-tile-5" v-bind:color="colors[5]" v-on:click="cb_action(5)"></v-btn></v-card>
        </v-col>
     </v-row>
    </v-container>
    <v-slider v-model="brightness" :max="255" label="Brightness" min-width="510" v-on:change="update_lc_config"></v-slider>
   </v-card>
   </v-col>

   <v-col cols="1">
   <v-btn v-bind:class="{primary: edit_mode}" v-on:click="toggle_edit_mode()"><v-icon>{{ settings_icon }}</v-icon></v-btn>
   </v-col>

  </v-row>
  </v-container>

  <photoshop-picker v-if="show_color_picker" v-model="color_definition" @cancel="color_picker_Cancel()" @ok="color_picker_OK()"/>
  </v-app>
</template>

<script>
import Vue from 'vue'
import axios from 'axios'
import VueAxios from 'vue-axios'
import { mdiWrench } from '@mdi/js'
import { Photoshop } from 'vue-color'

Vue.use(VueAxios, axios)


export default {
  name: 'app',
  data: () => ({
      brightness: 90,
      color_selector: 0,
      settings_icon: mdiWrench,
      edit_mode: false,
      setting_color: 0,
      show_color_picker: false,
      color_definition: {hex: '#194d3FF'},
      colors: ["#FF4500", "#FFFAF0", "#FF1493", "#00FF7F", "#0000CD", "#4B0082"]
  }),
  components: {
    'photoshop-picker': Photoshop
  },
  methods: {
    update_lc_config: function() {
      this.axios.post("/api/v1/lc/config", {
          brightness: this.brightness,
          color_id: this.color_selector
        })
        .then(data => {
          console.log(data);
        })
        .catch(error => {
          console.log(error);
        });
    },
    update_color_definition: function() {
      this.axios.post("/api/v1/lc/coldef", {
          hexvalue: this.colors[this.setting_color],
          color_id: this.setting_color
        })
        .then(data => {
          console.log(data);
        })
        .catch(error => {
          console.log(error);
        });
    },
    cb_action: function(val) {
      if (this.edit_mode) {
        this.edit_color(val);
      } else {
        this.set_color(val);
      }
    },
    set_color: function(val) {
      this.color_selector = val;
      this.update_lc_config();
    },
    edit_color: function(val) {
      this.setting_color = val;
      this.color_definition = {hex: this.colors[val]};
      this.show_color_picker = true;
    },
    color_picker_OK: function() {
      this.colors[this.setting_color] = this.color_definition.hex;
      this.update_color_definition();
      this.show_color_picker = false;
    },
    color_picker_Cancel: function() {
      this.show_color_picker = false;
    },
    toggle_edit_mode: function() {
      this.edit_mode = !this.edit_mode;
    }
  }
};
</script>

<style>
#app {
  font-family: Helvetica, Arial, sans-serif;
  text-align: center;
}
</style>
