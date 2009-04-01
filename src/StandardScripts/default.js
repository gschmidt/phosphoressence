function osc_receive_callback(path, value) {
    if (debug)
	print("[OSC]    Path: " + path + "   Value: " + value);
    bindings.controller_to_parameter(osc, path, value);
}

function joystick_receive_callback(path, value) {
    
    // -------------------------------------
    // Fixed settings for the langton bEATS!
    // -------------------------------------
    
    // RESET!!
    if (path == "/joystick0/button3" && value == 1) {
	print("Resetting to defaults!!\n>> ");
	p.reset_all();
	mv_a = 1.0;
	mv_x = 0;
	mv_y = 0;
	mv_l = 0;
	rot = -0.001;
	sx=0.999;
    }

    // Square shape enable
    if (path == "/joystick0/button1" && value == 1) {
	if (square_a) square_a = 0.0;
	else square_a = 1.0;
    }

    // wave enable
    if (path == "/joystick0/button2" && value == 1) {
	if (wave_enabled) {
	    wave_enabled = 0.0;
	} else {
	    wave_enabled = 1.0;
	}
    }

    // Invert
    if (path == "/joystick0/button4" && value == 1) {
	if (invert) invert = 0.0;
	else invert = 1.0;
    }

    // // Gamma
    // if (path == "/joystick0/hat0" && value == 4) 
    // 	gamma_coefficient = 1.0;
    // if (path == "/joystick0/hat0" && value == 1.0) 
    // 	gamma_coefficient = -1.0;
    // if (path == "/joystick0/hat0" && value == 0.0) 
    // 	gamma_coefficient = 0.0;

    // waveshape_frequency
    if (path == "/joystick0/hat0" && value == 4) 
    	wsfreq_coefficient = -1.0;
    if (path == "/joystick0/hat0" && value == 1) 
    	wsfreq_coefficient = 1.0;
    if (path == "/joystick0/hat0" && value == 8) 
    	sqfreq_coefficient = -1.0;
    if (path == "/joystick0/hat0" && value == 2) 
    	sqfreq_coefficient = 1.0;
    if (path == "/joystick0/hat0" && value == 0.0) {
    	wsfreq_coefficient = 0.0;
	sqfreq_coefficient = 0.0;
    }

    // wave_usedots
    if (path == "/joystick0/button7" && value == 1) {
	if (wave_usedots) wave_usedots = 0.0;
	else wave_usedots = 1.0;
    }

    // Rotation
    var rot_gain = 0.01;
    if (path == "/joystick0/axis0") {
	var delta = -(value-0.5) * rot_gain;
	if (Math.abs(value-0.5) > 0.05) {
	    rot += -delta;
	    if (rot > 0.785) rot = 0.785;
	    if (rot < -0.785) rot = -0.785;
	}
    }

    // Zoom
    var zoom_gain = 0.006;
    if (path == "/joystick0/axis1") {
	var delta = -(value-0.5) * zoom_gain;
	if (Math.abs(value-0.5) > 0.05) {
	    zoom += -delta;
	    if (zoom > 1.5) zoom = 1.5;
	    if (zoom < 0.5) zoom = 0.5;
	}
    }

    // Zoomexp
    if (path == "/joystick0/axis3") {
	var delta = (value-0.5)/10.0;
	if (Math.abs(value-0.5) > 0.05)
	    zoomexp -= delta;
	if (zoomexp < 0.25) zoomexp = 0.25; 
	if (zoomexp > 5.0) zoomexp = 5.0;
    }

    // Scaling
    if (path == "/joystick0/button21" && value == 1.0) 
	sx_coefficient = 1.0;
    if (path == "/joystick0/button21" && value == 0.0) 
	sx_coefficient = 0.0;
    if (path == "/joystick0/button19" && value == 1.0) 
	sx_coefficient = -1.0;
    if (path == "/joystick0/button19" && value == 0.0) 
	sx_coefficient = 0.0;

    if (path == "/joystick0/button20" && value == 1.0) 
	sy_coefficient = 1.0;
    if (path == "/joystick0/button20" && value == 0.0) 
	sy_coefficient = 0.0;
    if (path == "/joystick0/button18" && value == 1.0) 
	sy_coefficient = -1.0;
    if (path == "/joystick0/button18" && value == 0.0) 
	sy_coefficient = 0.0;

    // Center of rotation
    if (path == "/joystick0/button23" && value == 1.0) 
	cx_coefficient = 1.0;
    if (path == "/joystick0/button23" && value == 0.0) 
	cx_coefficient = 0.0;
    if (path == "/joystick0/button25" && value == 1.0) 
	cx_coefficient = -1.0;
    if (path == "/joystick0/button25" && value == 0.0) 
	cx_coefficient = 0.0;

    if (path == "/joystick0/button22" && value == 1.0) 
	cy_coefficient = 1.0;
    if (path == "/joystick0/button22" && value == 0.0) 
	cy_coefficient = 0.0;
    if (path == "/joystick0/button24" && value == 1.0) 
	cy_coefficient = -1.0;
    if (path == "/joystick0/button24" && value == 0.0) 
	cy_coefficient = 0.0;

    // Motion Vectors
    if (path == "/joystick0/button15" && value == 1.0) 
	if (mv_x == 0 || mv_y == 0) {
	    mv_x = 2;
	    mv_y = 2;
	} else if (mv_x < 64 && mv_y < 64) {
	    mv_x *= 2;
	    mv_y *= 2;
	}
    if (path == "/joystick0/button17" && value == 1.0) {
	if (mv_x == 2 || mv_y == 2) {
	    mv_x = 0;
	    mv_y = 0;
	} else {
	    mv_x /= 2;
	    mv_y /= 2;
	}
    }
    if (path == "/joystick0/button14" && value == 1.0) 
	mv_l_coeff = 1.0;
    if (path == "/joystick0/button14" && value == 0.0) 
	mv_l_coeff = 0.0;
    if (path == "/joystick0/button16" && value == 1.0) 
	mv_l_coeff = -1.0;
    if (path == "/joystick0/button16" && value == 0.0) 
	mv_l_coeff = 0.0;

    // Reset center of rotation & scaling
    if (path == "/joystick0/button5" && value == 1.0) {
	sx= 1.0;
	sy= 1.0;
	cx = 0.0;
	cy = 0.0;
	rot = 0;
	zoomexp = 1.0;
    }

    // Warp
    if (path == "/joystick0/button0" && value == 1.0) 
	warp_coefficient = 1.0;
    if (path == "/joystick0/button0" && value == 0.0) 
	warp_coefficient = 0.0;
    if (path == "/joystick0/button6" && value == 1.0) 
	warp_coefficient = -1.0;
    if (path == "/joystick0/button6" && value == 0.0) 
	warp_coefficient = 0.0;


    // Wave mode
    if (path == "/joystick0/button8" && value == 1) {
    	wave_mode = 0;
    	wave_enabled = 1;
    } else if (path == "/joystick0/button9" && value == 1) {
    	wave_mode = 1;
    	wave_enabled = 1;
    } else if (path == "/joystick0/button10" && value == 1) {
    	wave_mode = 2;
    	wave_enabled = 1;
    }

    // Inner border width
    if (path == "/joystick0/button11" && value == 1) {
	ib_size=0.0;
	ib_a = 0.0;
    } else if (path == "/joystick0/button12" && value == 1) {
    	ib_size = 1.0;
	ib_a = 1.0;
    } else if (path == "/joystick0/button13" && value == 1) {
    	ib_size = 10.0;
	ib_a = 1.0;
    }

    // Debugging
    if (joy_debug && (path.search("axis") == -1))
	print("[JOYSTICK]    Path: " + path + "   Value: " + value);

    bindings.controller_to_parameter(osc, path, value);
}

function pe_load() {
    
    // Load various javascript modules
    load(RESOURCES + "/scripts/PresetFunctions.js");
    load(RESOURCES + "/scripts/Bindings.js");

    // Set up bindings object
    bindings = new Bindings();
    run_preset = 0;

    // Switch on for debugging
    debug = 0;
    joy_debug = 0;
}

// Default initialization handler
//
// This function can be used to set up the environment in the JS
// virtual machine.  This is called once when the ScriptEngine is
// initialized.
function pe_initialize() {

    // Setup the receive callbacks for controllers
    osc.receive_callback = osc_receive_callback;
    joystick.receive_callback = joystick_receive_callback;

    // Set up some basic control bindings
    bindings.add(osc, "/1/fader1", "decay", 0.15, 1.1, 0.95, "log10");
    bindings.add(osc, "/1/fader2", "zoom", 0.5, 1.5, 1.0);
    bindings.add(osc, "/1/fader3", "zoomexp", 0.25, 5.0, 1.0, "log10");  
    bindings.add(osc, "/1/fader4", "rot", -0.785, 0.785, 0.0);

    bindings.add(osc, "/2/fader1", "sx", 0.5, 1.5, 1.0);
    bindings.add(osc, "/2/fader2", "sy", 0.5, 1.5, 1.0);
    bindings.add(osc, "/2/fader3", "dx", -0.5, 0.5, 0.0);
    bindings.add(osc, "/2/fader4", "dy", -0.5, 0.5, 0.0);
    bindings.add(osc, "/2/fader5", "cx", -1.5, 1.5, 0.0);
    bindings.add(osc, "/2/fader6", "cy", -1.5, 1.5, 0.0);
    bindings.add(osc, "/2/toggle7", "wave_enabled", 0.0, 1.0, 0.0);
    bindings.add(osc, "/2/fader7", "wave_frequency", 0.01, 300, 100, "log10");
    bindings.add(osc, "/2/toggle8", "square_a", 0.0, 1.0, 1.0);
    bindings.add(osc, "/2/fader8", "square_frequency", 0.001, 1.0, 0.03);

    bindings.add(osc, "/3/fader1", "warp", 0.0, 2.0, 0.0);
    bindings.add(osc, "/3/fader2", "warp_speed", 0.0, 1.0, 0.5);
    bindings.add(osc, "/3/fader3", "warp_scale", 0.01, 1.0, 0.5);
    bindings.add(osc, "/3/fader4", "mv_x", 0, 64.0, 64.0);
    bindings.add(osc, "/3/fader5", "mv_y", 0, 48.0, 48.0);
    bindings.add(osc, "/3/fader6", "mv_dx", 0.0, 0.1, 0.0);
    bindings.add(osc, "/3/fader7", "mv_dy", 0.0, 0.1, 0.0);
    bindings.add(osc, "/3/fader8", "mv_l", 0.01, 0.2, 0.01);

    // pe_parameters().add_parameter("rd_width", "/3/fader1", 0.0, 50.0, 1.0);
    // pe_parameters().add_parameter("rd_D_g", "/3/fader2", 0.0, 0.5, 0.25);
    // pe_parameters().add_parameter("rd_D_b", "/3/fader3", 0.0, 0.5, 0.0625);
    // pe_parameters().add_parameter("rd_s", "/3/fader4", 0.0, 0.05, 0.03125);
    // pe_parameters().add_parameter("rd_beta", "/3/fader5", 0.0, 24.0, 12);
    // pe_parameters().add_parameter("rd_blur", "/3/fader8", 0.0, 4.0, 0.0);

    // Langton bEATS
    // bindings.add(osc, "/joystick0/axis2", "decay", 0.15, 1.1, 0.95);
    // bindings.add(osc, "/joystick0/axis4", "warp", 2.0, 0.0, 0.0);
    // bindings.add(osc, "/joystick0/axis5", "warp_scale", 2.0, 0.16);

    sx_coefficient = 0.0;
    sy_coefficient = 0.0;
    cx_coefficient = 0.0;
    cy_coefficient = 0.0;
    warp_coefficient = 0.0;
    gamma_coefficient = 0.0;
    wsfreq_coefficient = 0.0;
    sqfreq_coefficient = 0.0;
    color_shift_coefficient = 0.0;
    mv_l_coeff = 0.0;

    mv_a = 1.0;
    mv_x = 0;
    mv_y = 0;
    mv_l = 0;
    rot = -0.001;
    sx=0.999;

    
    // Load Milkdrop Presets
    //    load(RESOURCES + "/presets/milkdrop/milk_presets.js");
}


// Default render callback
//
// This method is called by the GraphicsEngine just prior to rendering
// each frame.  By overriding this method, programmers can animate
// PhosphorEssence parameters.
function pe_render() {
    
    // if ( run_preset ) {

    // 	// Milkdrop preset mode
    // 	if ( preset = current_preset() )
    // 	    preset();

    // } else {

    // 	// Update scaling
    // 	var scaling_stepsize = 1/100.0;
    // 	sx += scaling_stepsize * sx_coefficient;
    // 	sy += scaling_stepsize * sy_coefficient;
    // 	if (sx > 1.5) sx = 1.5;
    // 	if (sx < 0.5) sx = 0.5;
    // 	if (sy > 1.5) sy = 1.5;
    // 	if (sy < 0.5) sy = 0.5;

    // 	// Update center of rotation
    // 	var crot_stepsize = 1/100.0;
    // 	cx += crot_stepsize * cx_coefficient;
    // 	cy += crot_stepsize * cy_coefficient;
    // 	if (cx > 1.5) cx = 1.5;
    // 	if (cx < -1.5) cx = -1.5;
    // 	if (cy > 1.5) cy = 1.5;
    // 	if (cy < -1.5) cy = -1.5;

    // 	// Update warp
    // 	var warp_stepsize = 1/30.0;
    // 	warp += warp_stepsize * warp_coefficient;
    // 	if (warp > 2.0) warp = 2.0;
    // 	if (warp < 0.0) warp = 0.0;

    // 	// Update gamma
    // 	// var gamma_stepsize = 1/100.0;
    // 	// gamma += gamma_stepsize * gamma_coefficient;
    // 	// if (gamma > 1.5) gamma = 1.5;
    // 	// if (gamma < 0.9) gamma = 0.9;

    // 	// Update wsfreq 
    // 	var wsfreq_stepsize = 1.1;
    // 	if (wsfreq_coefficient > 0)
    // 	    wave_frequency *= wsfreq_stepsize;
    // 	else if (wsfreq_coefficient < 0)
    // 	    wave_frequency /= wsfreq_stepsize;
    // 	if (wave_frequency > 400) wave_frequency = 400;
    // 	if (wave_frequency < 0.01) wave_frequency = 0.01;

    // 	// Update sqfreq 
    // 	// var sqfreq_stepsize = 1.05;
    // 	// if (sqfreq_coefficient > 0)
    // 	//     square_frequency *= sqfreq_stepsize;
    // 	// else if (sqfreq_coefficient < 0)
    // 	//     square_frequency /= sqfreq_stepsize;
    // 	// if (square_frequency > 0.05) square_frequency = 0.05;
    // 	// if (square_frequency < 0.001) square_frequency = 0.001;
    // 	// print(square_frequency);

    // 	// Update color_shift
    // 	var color_shift_stepsize = 1/100.0;
    // 	color_shift += color_shift_stepsize * color_shift_coefficient;
    // 	if (color_shift > 1.0) color_shift = 1.0;
    // 	if (color_shift < -1.0) color_shift = -1.0;

    // 	// Update MV length
    // 	var mv_l_stepsize = 1/20.0;
    // 	mv_l += mv_l_stepsize * mv_l_coeff;
    // 	if (mv_l > 2.0) mv_l = 2.0;
    // 	if (mv_l < 0.0) mv_l = 0.0;
	
    	// Cycle through OB colors
    	ib_r = 0.5;
    	ib_g = 0.5;
    	ib_b = 0.5;
    	ib_r += 0.35 * Math.sin(7/10.0*time);
    	ib_g += 0.35 * Math.sin(11/10.0*time);
    	ib_b += 0.35 * Math.sin(13/10.0*time);

    	// Cycle through Wave colors
    	wave_brighten = 1.0;
    	wave_r=0.65;
    	wave_g=0.65;
    	wave_b=0.65;
    	wave_r = wave_r + 0.350*( 0.60*Math.sin(0.742*time) + 0.40*Math.sin(1.021*time) );
    	wave_g = wave_g + 0.350*( 0.60*Math.sin(0.703*time) + 0.40*Math.sin(0.969*time) );
    	wave_b = wave_b + 0.350*( 0.60*Math.sin(1.090*time) + 0.40*Math.sin(0.963*time) );
	
    	// Cycle through waveshape colors
    	square_r=0.65;
    	square_g=0.65;
    	square_b=0.65;
    	square_r += 0.35 * Math.sin(17/10.0*time);
    	square_g += 0.35 * Math.sin(13/10.0*time);
    	square_b += 0.35 * Math.sin(12/10.0*time);

    	// Cycle through motion vecton colors
    	mv_r=0.65;
    	mv_g=0.65;
    	mv_b=0.65;
    	mv_r += 0.5 * Math.sin(15/10.0*time);
    	mv_g += 0.5 * Math.sin(19/10.0*time);
    	mv_b += 0.5 * Math.sin(14/10.0*time);

    // 	// wave_x = 0.5;
    // 	// wave_y = 0.5;
    // 	// wave_x = wave_x + 0.500*( 0.60*Math.sin(2.121*time) + 0.40*Math.sin(1.621*time) );
    // 	// wave_y = wave_y + 0.500*( 0.60*Math.sin(1.742*time) + 0.40*Math.sin(2.322*time) );
    //   }
}


