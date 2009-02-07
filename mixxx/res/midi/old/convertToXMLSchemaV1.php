#!/usr/bin/env php
<?php
/***************************************************************************
 * This small script converts Mixxx MIDI Presets from the 1.6.X XML format *
 * to the new schema v1 format introduced in Mixxx 1.6.2.                  *
 *                                                                         *
 * The changes applied are thus:                                           *
 *    1) Add MixxxMIDIPreset tag                                           *
 *    2) Add info tag and children                                         *
 *    3) convert midichan and midino values to hexadecimal                 *
 *                                                                         *
 * Contributed to Mixxx by Mad Jester (Phillip Whelan)                     *
 *      pwhelan <at> exis.cl                                               *
 *                                                                         *
 **************************************************************************/

/* Indent the XML file for readability */
function indent($xml)
{
	$doc = new DOMDocument('1.0');
	
	$doc->preserveWhiteSpace = false;
	$doc->loadXML($xml->asXML());
	$doc->formatOutput = true;
	
	return $doc->saveXML();
}


function fileToControllerName($file)
{
    $name = substr($file, 0, strpos($file, '.'));
    
    return preg_replace('/_/', ' ', $name);
}


/* Carry out the actual conversion */
function convert_xml($file)
{
    ($old = simplexml_load_file($file)) || die("Unable to Load MIDI XML File: $file\n");

    // Create the file with default values
    $new = simplexml_load_string(
    "<MixxxMIDIPreset schemaVersion=\"1\" mixxxVersion=\"1.6.2+\">
      <info>
        <name>{$file}</name>
        <author>Auto-converted by madjesta's PHP script</author>
        <description>Auto-conversion of the file {$file}</description>
      </info>
      <controller id=\"".fileToControllerName($file)."\" port=\"Port\">
      </controller>
    </MixxxMIDIPreset>
    "
    );

    
    // Add Controls to Controller Node
    $controls = $new->controller->addChild("controls");
    
    // get the control children
    $oldControls = $old->children();

    // Here we perfom the actual conversion and moving of the values to the
    // new file.
    foreach( $oldControls->children() as $oldControl ) {
        $control = $controls->addChild("control");
        
        foreach($oldControl->children() as $k => $v) {
            if (($k == "midino") || ($k == "midichan"))
                $v = sprintf("0x%02x", (integer)$v);
            
            $control->addChild($k, $v);
        }
    }
    
    return indent($new);
}

// Print out the files conversion
print convert_xml($argv[1]);

?>