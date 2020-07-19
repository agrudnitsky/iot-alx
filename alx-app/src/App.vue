<template>
  <v-app>
    <v-app-bar app>
      Awesome Lights Xtreme - v.{{ version }}
    </v-app-bar>

    <v-main>
      <v-container>

        <v-card class="mx-auto" min-width="350">
          Mode
          <v-btn-toggle mandatory v-model="mode">
            <v-btn v-on:click="set_mode(0)" value="0">Constant</v-btn>
            <v-btn v-on:click="set_mode(6)" value="6">Time-Dependent</v-btn>
            <v-btn v-on:click="set_mode(7)" value="7">XMas</v-btn>
          </v-btn-toggle>

        </v-card>

        <v-card class="mx-auto" min-width="350">
          <v-container>
            <v-row>
              <v-col><v-btn v-on:click="palette_advance(-1)"><v-icon large>{{ icon_left }}</v-icon></v-btn></v-col>
              <v-col>Palette {{ color_palette }} / {{ num_palettes-1 }}</v-col>
              <v-col><v-btn v-on:click="palette_advance(1)"><v-icon large>{{ icon_right }}</v-icon></v-btn></v-col>
            </v-row>
            <v-row>
              <v-col>
                <v-card tile>
                  <v-row v-for="offset in 2" v-bind:key="offset" align="center" justify="center">
                    <v-col v-for="idx in 3" v-bind:key="idx">
                      <v-card class="px-auto" v-bind:raised="color_id==(offset-1)*3+idx-1"><v-btn tile v-bind:color="colors[color_palette][(offset-1)*3+idx-1]" v-on:click="cb_action((offset-1)*3+idx-1)"></v-btn></v-card>
                    </v-col>
                  </v-row>
                </v-card>
              </v-col>
            </v-row>
          </v-container>
          <v-slider v-model="brightness" max="255" label="Bright" min-width="510" v-on:change="update_lc_config"></v-slider>
        </v-card>

        <v-row>
          <v-col>
            <v-btn v-bind:class="{primary: edit_mode}" v-on:click="toggle_edit_mode()"><v-icon>{{ icon_settings }}</v-icon></v-btn>
          </v-col>
          <v-col>
            <v-btn v-on:click="toggle_remote_onoff()" v-bind:class="{primary: remote_onoff}"><v-icon>{{ icon_remote_onoff }}</v-icon></v-btn>
          </v-col>
          <v-col>
            <v-btn v-on:click="refresh_config()" bottom><v-icon>{{ icon_refresh }}</v-icon></v-btn>
          </v-col>
        </v-row>
      </v-container>

      <photoshop-picker v-if="show_color_picker" v-model="color_definition" @cancel="color_picker_Cancel()" @ok="color_picker_OK()"/>
    </v-main>
  </v-app>
</template>

<script>
import Vue from 'vue'
import axios from 'axios'
import VueAxios from 'vue-axios'
import { mdiWrench, mdiLightbulbOn, mdiLightbulb, mdiChevronLeft, mdiChevronRight, mdiReload } from '@mdi/js'
import { Photoshop } from 'vue-color'

Vue.use(VueAxios, axios)

export default {
  name: 'app',
  data: () => ({
    version: process.env.VUE_APP_VERSION,
    mode: '7',
    brightness: 90,
    color_id: 0,
    remote_onoff: 1,
    num_palettes: 2,
    color_palette: 0,
    edit_mode: false,
    show_color_picker: false,
    color_definition: { hex: '#194d3FF' },
    colors: [['#FF4500', '#FFFAF0', '#FF1493', '#00FF7F', '#0000CD', '#4B0082'], ['#FF0000', '#00FF00', '#0000FF', '#FFFFFF', '#0000000', '#888888']],
    icon_settings: mdiWrench,
    icon_remote_onoff: mdiLightbulbOn,
    icon_refresh: mdiReload,
    icon_left: mdiChevronLeft,
    icon_right: mdiChevronRight
  }),
  components: {
    'photoshop-picker': Photoshop
  },
  mounted () {
    this.refresh_config()
  },
  methods: {
    refresh_config: function () {
      this.axios
        .get('/api/v1/lc/getconfig')
        .then(response => {
          this.mode = response.data.mode.toString(),
          this.brightness = response.data.brightness,
          this.color_id = response.data.color,
          this.color_palette = response.data.color_palette,
          this.remote_onoff = response.data.remote_onoff,
          this.num_palettes = response.data.num_palettes,
          this.icon_remote_onoff = response.data.remote_onoff ? mdiLightbulbOn : mdiLightbulb
        })
        .catch(error => { console.log(error) })
      this.axios
        .get('/api/v1/lc/getcols')
        .then(response => { this.colors = response.data.cols, console.log(response) })
        .catch(error => { console.log(error) })
    },
    update_lc_config: function () {
      this.axios
        .post('/api/v1/lc/setconfig', {
          mode: this.mode,
          brightness: this.brightness,
          color_id: this.color_id,
          color_palette: this.color_palette,
          remote_onoff: this.remote_onoff
        })
        .then(data => {
          console.log(data)
        })
        .catch(error => {
          console.log(error)
        })
    },
    update_color_definition: function () {
      this.axios
        .post('/api/v1/lc/coldef', {
          hexvalue: this.colors[this.color_palette][this.color_id],
          color_palette: this.color_palette,
          color_id: this.color_id
        })
        .then(data => {
          console.log(data)
        })
        .catch(error => {
          console.log(error)
        })
    },
    palette_advance: function (val) {
      if (0 === this.color_palette && -1 === val) {
        this.color_palette = this.num_palettes - 1
      } else {
        this.color_palette = (this.color_palette + val) % this.num_palettes
      }
    },
    set_mode: function (val) {
      this.mode = val
      this.update_lc_config()
    },
    cb_action: function (val) {
      if (this.edit_mode) {
        this.edit_color(val)
      } else {
        this.set_color(val)
      }
    },
    set_color: function (val) {
      this.color_id = val
      this.update_lc_config()
    },
    edit_color: function (val) {
      this.color_id = val
      this.color_definition = { hex: this.colors[this.color_palette][val] }
      this.show_color_picker = true
    },
    color_picker_OK: function () {
      this.colors[this.color_palette][this.color_id] = this.color_definition.hex
      this.update_color_definition()
      this.show_color_picker = false
    },
    color_picker_Cancel: function () {
      this.show_color_picker = false
    },
    toggle_edit_mode: function () {
      this.edit_mode = !this.edit_mode
    },
    toggle_remote_onoff: function () {
      this.remote_onoff = this.remote_onoff ? 0 : 1
      if (this.remote_onoff) {
        this.remote_onoff_icon = mdiLightbulbOn
      } else {
        this.remote_onoff_icon = mdiLightbulb
      }
      this.update_lc_config()
    }

  }
}
</script>

<style>
#app {
  font-family: Helvetica, Arial, sans-serif;
  text-align: center;
}
</style>
