#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0; // A single pre-rendered cell texture (cellRevealRT)
uniform vec2 gridSize;      // Dimensions of the grid (cols, rows)
uniform float cellSize;     // Size of one cell in pixels
uniform float margin;       // Margin between cells in pixels
uniform vec4 bgColor;       // Background color (bgSlice) normalized

void main() {
    // Calculate the absolute pixel position within the grid area.
    // fragTexCoord is [0,1] across the entire drawn rectangle.
    vec2 totalSize = gridSize * cellSize;
    vec2 pixelPos = fragTexCoord * totalSize;

    // Calculate the position relative to the current cell's top-left corner.
    vec2 posInCell = mod(pixelPos, cellSize);

    // Check if the pixel falls within the margin area between cells.
    if (posInCell.x < margin || posInCell.x > cellSize - margin ||
        posInCell.y < margin || posInCell.y > cellSize - margin) {
        // It's in the margin, draw the background color.
        finalColor = bgColor;
    } else {
        // It's inside a cell. Sample the single cell texture.
        // Map posInCell from [margin, cellSize-margin] to [0, 1] for UV coords.
        float cellInnerSize = cellSize - 2.0 * margin;
        vec2 uvInCell = (posInCell - vec2(margin)) / cellInnerSize;

        // The texture from a RenderTexture is flipped vertically. Flip it back.
        uvInCell.y = 1.0 - uvInCell.y;

        // Clamp UVs slightly to prevent bleeding artifacts at cell edges due to precision.
        uvInCell = clamp(uvInCell, 0.01, 0.99);

        finalColor = texture(texture0, uvInCell);
    }
    // Mix with the vertex color (usually WHITE).
    finalColor *= fragColor;
}