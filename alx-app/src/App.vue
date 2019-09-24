<template>
  <v-app>
   <v-card class="mx-auto" min-width="350">
    <v-container>
      <v-row>
        <v-col>
          <v-card><v-btn tile color="orange" v-on:click="set_color(0)">0</v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="white" v-on:click="set_color(1)">1</v-btn></v-card>
        </v-col>
        <v-col>
          <v-card><v-btn tile color="purple" v-on:click="set_color(2)">2</v-btn></v-card>
        </v-col>
      </v-row>
    </v-container>
    <v-slider v-model="brightness" :max="255" label="Brightness" min-width="510" v-on:change="update_lc_config"></v-slider>
    <v-btn fab dark large color="red accent-4" @click="update_lc_config">
      <v-icon dark>mdi-send</v-icon>
    </v-btn>
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
