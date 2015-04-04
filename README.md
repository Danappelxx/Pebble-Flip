# Pebble-Flip
An instagram client for the pebble smartwatch.

# Usage
After you install Pebble-Flip, press the settings button for Pebble-Flip on your smartphone pebble app. Follow the instructions that you are redirected to. Afterwards, exit the application on your pebble and enter it again.

If you did everything correctly, wait several seconds (5 should be enough) after launching Pebble-Flip and press the down arrow. After a few seconds, the first image on your instagram feed will load. Pressing the down button will take you to the next image, while pressing the up button will take you to the previous image.

# Notes
As of now, the known bugs are:
* The app crashes occasionally due to memory errors (unfortunately, seems to be out of my control).
* Attempting to go to the previous image on your feed while on the first image of your feed will freeze the application
* There is no indication of when the image urls are finished loading. This generally takes under 5 seconds, but you can't be certain unless you look at the app logs.

# Composition
The backend (server) is created using uWSGI with python and nGINX. The server files are in the folder 'backend'.



# License
This project is largely built on an example project by Pebble called [Pebble Faces](https://github.com/pebble-examples/pebble-faces) which is protected under the MIT License.
