<template>
  <v-app>
   <v-card class="mx-auto" min-width="350">
    <v-container>
      <v-row>
        <v-col>
          <v-card><v-btn tile color="#FF4500" v-on:click="set_color(0)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="#FFFAF0" v-on:click="set_color(1)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="#FF1493" v-on:click="set_color(2)"></v-btn></v-card>
        </v-col>
      </v-row>
      <v-row>
        <v-col>
          <v-card><v-btn tile color="#00FF7F" v-on:click="set_color(3)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="#0000CD" v-on:click="set_color(4)"></v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="#4B0082" v-on:click="set_color(5)"></v-btn></v-card>
        </v-col>
      </v-row>
    </v-container>
    <v-slider v-model="brightness" :max="255" label="Brightness" min-width="510" v-on:change="update_lc_config"></v-slider>
   </v-card>
  </v-app>
</template>

<script>
import Vue from 'vue'
import axios from 'axios'
import VueAxios from 'vue-axios'

Vue.use(VueAxios, axios)


export default {
  name: 'app',
  data: () => ({
      brightness: 90,
      color_selector: 0
  }),
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
    set_color: function(val) {
      this.color_selector = val;
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
