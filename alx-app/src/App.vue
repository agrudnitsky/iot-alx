<template>
  <v-app>
    <v-app-bar app>
      Awesome Lights Xtreme - v.{{ version }}
    </v-app-bar>

   <v-content>
   <v-container>
   <v-row>

   <v-col justify-self="end">
   <v-card class="mx-auto" min-width="350">
    <v-container>
      <v-row v-for="offset in 2" v-bind:key="offset">
        <v-col v-for="idx in 3" v-bind:key="idx">
          <v-card v-bind:raised="color_id==(offset-1)*3+idx-1"><v-btn tile v-bind:color="colors[(offset-1)*3+idx-1]" v-on:click="cb_action((offset-1)*3+idx-1)"></v-btn></v-card>
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

  <v-btn v-on:click="toggle_remote_onoff()" v-bind:class="{primary: remote_onoff}"><v-icon>{{ remote_onoff_icon }}</v-icon></v-btn>
  </v-container>

  <photoshop-picker v-if="show_color_picker" v-model="color_definition" @cancel="color_picker_Cancel()" @ok="color_picker_OK()"/>
  </v-content>
  </v-app>
</template>

<script>
import Vue from 'vue'
import axios from 'axios'
import VueAxios from 'vue-axios'
import { mdiWrench, mdiLightbulbOn, mdiLightbulb } from '@mdi/js'
import { Photoshop } from 'vue-color'

Vue.use(VueAxios, axios)


export default {
  name: 'app',
  data: () => ({
      version: process.env.VUE_APP_VERSION,
      brightness: 90,
      color_id: 0,
      remote_onoff: 1,
      settings_icon: mdiWrench,
      remote_onoff_icon: mdiLightbulbOn,
      edit_mode: false,
      show_color_picker: false,
      color_definition: {hex: '#194d3FF'},
      colors: ["#FF4500", "#FFFAF0", "#FF1493", "#00FF7F", "#0000CD", "#4B0082"]
  }),
  components: {
    'photoshop-picker': Photoshop
  },
  mounted() {
    this.refresh_config()
  },
  methods: {
    refresh_config: function() {
      axios
      .get("/api/v1/lc/getconfig")
      .then(response => {
        this.brightness = response.data.brightness,
        this.color_id = response.data.color,
        this.remote_onoff = response.data.remote_onoff,
        this.remote_onoff_icon = response.data.remote_onoff ? mdiLightbulbOn : mdiLightbulb
       })
      .catch(error => {console.log(error)}),
    axios
      .get("/api/v1/lc/getcols")
      .then(response => {this.colors = response.data.cols
      }).catch(error => {console.log(error)})
    },
    update_lc_config: function() {
      this.axios.post("/api/v1/lc/setconfig", {
          brightness: this.brightness,
          color_id: this.color_id,
          remote_onoff: this.remote_onoff
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
          hexvalue: this.colors[this.color_id],
          color_id: this.color_id
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
      this.color_id = val;
      this.update_lc_config();
    },
    edit_color: function(val) {
      this.color_id = val;
      this.color_definition = {hex: this.colors[val]};
      this.show_color_picker = true;
    },
    color_picker_OK: function() {
      this.colors[this.color_id] = this.color_definition.hex;
      this.update_color_definition();
      this.show_color_picker = false;
    },
    color_picker_Cancel: function() {
      this.show_color_picker = false;
    },
    toggle_edit_mode: function() {
      this.edit_mode = !this.edit_mode;
    },
    toggle_remote_onoff: function() {
      this.remote_onoff = this.remote_onoff ? 0 : 1;
      if (this.remote_onoff) {
        this.remote_onoff_icon = mdiLightbulbOn;
      } else {
        this.remote_onoff_icon = mdiLightbulb;
      }
      this.update_lc_config();
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
