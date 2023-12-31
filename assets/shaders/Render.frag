#version 450 core
out vec4 frag_color;

in vec2 texCoords;
uniform sampler2D map;

void main()
{
    vec3 color = texture2D(map, texCoords).rgb;

    if(color == vec3(1.0))
    {
        discard;
        // frag_color = vec4(0.0);
    }
    else
    {
        frag_color = vec4(color, 1.0);
        // gl_FragDepth = -0.5;
    }
}