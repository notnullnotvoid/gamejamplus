#include "imm.hpp"
#include "stb_image.h"
#include "platform.hpp"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////
/// FONTS                                                                    ///
////////////////////////////////////////////////////////////////////////////////

//TODO: go over this code and see if it can be improved
//TODO: switch to pre-processing font files at compile time?

Font load_font(const char * bmfont) {
    enum BlockType {
        OTHER, COMMON, PAGE, CHAR,
    };

    Font font = {};
    font.kernBias = -8; //TODO: compute this better
    font.chars = (Char *) malloc(128 * sizeof(Char));
    memset(font.chars, 0, 128 * sizeof(Char));

    char * text = read_entire_file(bmfont);

    if (text == nullptr) {
        fprintf(stderr, "ERROR: Could not load file %s\n", bmfont);
        fflush(stdout);
        exit(1);
    }

    char * relativePath = nullptr;
    Char * ch = nullptr;

    //kerning pairs appear to have very little visual impact, especially at small glyph sizes,
    //so we leave them out of our implementation for size/memory/simplicity/performance reasons

    //simple stateful parsing algorithm which ignores line breaks because it can.
    //for now we assume all glyphs are contained in one image, and ignore non-ascii chars
    //(only tested on the output of Hiero - conformance with other bmfont files is not guaranteed)
    BlockType type = OTHER;
    char * token = strtok(text, " \t\r\n");
    while (token != nullptr) {
             if (!strcmp(token, "common" )) { type = COMMON;  }
        else if (!strcmp(token, "page"   )) { type = PAGE;    }
        else if (!strcmp(token, "char"   )) { type = CHAR;    }
        else if (type != OTHER) {
            //parse key-value pair
            char * eq = strchr(token, '=');
            if (eq == nullptr) {
                type = OTHER;
            } else {
                *eq = '\0';
                char * key = token;
                char * value = eq + 1;

                switch (type) {
                    case COMMON: {
                        if (!strcmp(key, "lineHeight")) {
                            font.lineHeight = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "base")) {
                            font.base = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "scaleW")) {
                            font.scaleW = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "scaleH")) {
                            font.scaleH = strtol(value, nullptr, 10);
                        }
                    } break;
                    case PAGE: {
                        if (!strcmp(key, "file")) {
                            *strchr(value + 1, '"') = '\0';
                            relativePath = value + 1;
                        }
                    } break;
                    case CHAR: {
                        if (!strcmp(key, "id")) {
                            int id = strtol(value, nullptr, 10);
                            if (id < 128) {
                                ch = font.chars + id;
                            } else {
                                printf("NON-ASCII CHAR IN FONT: %d\n", id);
                                //overwriting nul char is safe because it is never printed
                                //TODO: properly extend this to support all of unicode
                                ch = font.chars;
                            }
                        } else if (!strcmp(key, "x")) {
                            ch->x = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "y")) {
                            ch->y = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "width")) {
                            ch->width = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "height")) {
                            ch->height = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "xoffset")) {
                            ch->xoffset = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "yoffset")) {
                            ch->yoffset = strtol(value, nullptr, 10);
                        } else if (!strcmp(key, "xadvance")) {
                            ch->xadvance = strtol(value, nullptr, 10);
                        }
                    } break;
                    case OTHER: {} //unreachable case included to suppress warning
                }
            }
        }

        token = strtok(nullptr, " \t\r\n");
    }

    //make "absolute" (root-relative) path from file-relative path
    const char * slash = strrchr(bmfont, '/');
    int folderPathLen = slash - bmfont + 1;
    char * imagePath = (char *) malloc(folderPathLen + strlen(relativePath) + 1);
    strncpy(imagePath, bmfont, folderPathLen);
    strcpy(imagePath + folderPathLen, relativePath);

    free(text); //not used past this point

    //load image
    font.tex = load_texture(imagePath);
    assert(font.tex.handle);

    free(imagePath); //not used past this point

    gl_error("FONT_LOAD");

    return font;
}

void free_font(Font &font) {
    free(font.chars);
    font.chars = nullptr;
    glDeleteTextures(1, &font.tex.handle);
    font.tex.handle = 0;
}

////////////////////////////////////////////////////////////////////////////////
/// OTHER STUFF                                                              ///
////////////////////////////////////////////////////////////////////////////////

void Imm::init(int size) {
    *this = {}; //zero-initialize
    program = create_program_from_files("res/imm.vert", "res/imm.frag");

    //allocate data
    data = (Vertex *) malloc(sizeof(Vertex) * size);

    //allocate VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //allocate VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    //setup vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void *) (3 * 4));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (4 * 4));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) (6 * 4));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    total = size;
    used = 0;

    //set general properties
    matrix = { 1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1, };
    depth = 1.0f;
    r = 1;
    cap = SQUARE;
    join = MITER;
}

void Imm::finalize() {
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    free(data);
}

void Imm::begin(int width, int height) {
    matrix.m00 = 2.0f / width;
    matrix.m11 = 2.0f / height;
    matrix.m03 = -1.0f;
    matrix.m13 = -1.0f;

    ww = width;
    wh = height;

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    depth = 1.0f;
}

void Imm::incrementDepth() {
    //by resetting the depth buffer when needed, we are able to have arbitrarily many layers
    if (depth < -0.9999f) {
        flush();
        glClear(GL_DEPTH_BUFFER_BIT);
        //depth test will fail at depth = 1.0 after clearing the depth buffer,
        //but since we always increment before drawing anything, this should be okay
        depth = 1.0f;
    }

    //use smallest safe depth increment for 16-bit linear depth buffer
    depth -= powf(2, -14);
}

void Imm::backgroundGradient(Color color1, Color color2, float direction) {
    assert(direction >= 0);
    float theta = fmodf(direction, HALF_PI);
    float side1 = cosf(theta) * ww + sinf(theta) * wh;
    float side2 = sinf(theta) * ww + cosf(theta) * wh;

    Vec2 center = vec2(ww / 2, wh / 2);
    Mat2 rot = rotate(theta);
    Vec2 p1 = center + rot * vec2(-side1 / 2, -side2 / 2);
    Vec2 p2 = center + rot * vec2( side1 / 2, -side2 / 2);
    Vec2 p3 = center + rot * vec2( side1 / 2,  side2 / 2);
    Vec2 p4 = center + rot * vec2(-side1 / 2,  side2 / 2);

    Color c1 = color1, c2 = color1, c3 = color2, c4 = color2;
    int quadrant = (int) (floorf(direction / HALF_PI)) % 4;
    switch (quadrant) {
        case 0: c1 = c2 = color1; c3 = c4 = color2; break;
        case 1: c2 = c3 = color1; c4 = c1 = color2; break;
        case 2: c3 = c4 = color1; c1 = c2 = color2; break;
        case 3: c4 = c1 = color1; c2 = c3 = color2; break;
    }

    incrementDepth();
    check(6);
    vertex(p1.x, p1.y, c1);
    vertex(p2.x, p2.y, c2);
    vertex(p3.x, p3.y, c3);
    vertex(p1.x, p1.y, c1);
    vertex(p3.x, p3.y, c3);
    vertex(p4.x, p4.y, c4);
}

//WARNING: NOT RE-ENTRANT - calls begin() and end() so don't call this during an existing begin()/end() pair!
//WARNING: sets GL state: stencil test, depth test, stencil func, stencil ops
void Imm::debugStencilVis(Color c, uint8_t val) {
    //do the visualization
    begin(1, 1);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_EQUAL, val, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    rect(0, 0, 1, 1, c);
    end();
}

void Imm::flush() {
    if (used == 0) {
        return;
    }

    //bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex.handle);

    glUseProgram(program);
    GLint scaleLocation = glGetUniformLocation(program, "scale");
    glUniform2f(scaleLocation, 1.0f / tex.width, 1.0f / tex.height);

    //upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, used * sizeof(Vertex), data, GL_DYNAMIC_DRAW);
    //BufferSubData is way slower than BufferData for some reason, at least with small batches
    //I have no idea why - look into this! (maybe ask on StackOverflow)
    // glBufferSubData(GL_ARRAY_BUFFER, 0, flat.used * sizeof(FlatVert), flat.data);

    glUseProgram(program);
    glBindVertexArray(vao);

    GLint transformLocation = glGetUniformLocation(program, "transform");
    glUniformMatrix4fv(transformLocation, 1, GL_TRUE, (float *)(&matrix));

    glDrawArrays(GL_TRIANGLES, 0, used);

    //unbind things (this isn't strictly necessary but I don't think it hurts any)
    glBindVertexArray(0);
    glUseProgram(0);

    //reset state
    used = 0;

    gl_error("imm flush");
}

void Imm::check(int newVerts) {
    if (used + newVerts > total) {
        flush();
    }
}

void Imm::vertex(float x, float y, Color color) {
    data[used] = { x, y, depth, color, 0, 0, 0 };
    used += 1;
}

void Imm::vertex(float x, float y, Color color, float u, float v) {
    data[used] = { x, y, depth, color, u, v, 1 };
    used += 1;
}

void Imm::triangle(float x1, float y1, float x2, float y2, float x3, float y3, Color fill) {
    check(3);

    vertex(x1, y1, fill);
    vertex(x2, y2, fill);
    vertex(x3, y3, fill);
}

void Imm::drawText(const char * text, float x, float y, float s, Color color) {
    drawText(text, x, y, s, color, strlen(text));
}

void Imm::drawTextCentered(const char * text, float x, float y, float s, Color color) {
    float width = text_width(font, s, text);
    drawText(text, x - width / 2, y, s, color);
}

void Imm::drawText(const char * text, float x, float y, float s, Color color, int len) {
    useTexture(font.tex);
    for (int i = 0; i < len; ++i) {
        incrementDepth();

        Char * ch = font.chars + text[i];

        float x0 = x + ch->xoffset * s, y0 = y + ch->yoffset * s;
        float x1 = x0 + ch->width * s, y1 = y0 + ch->height * s;
        float u0 = ch->x, v0 = ch->y;
        float u1 = u0 + ch->width, v1 = v0 + ch->height;

        check(6);

        vertex(x0, y0, color, u0, v0);
        vertex(x0, y1, color, u0, v1);
        vertex(x1, y0, color, u1, v0);
        vertex(x0, y1, color, u0, v1);
        vertex(x1, y0, color, u1, v0);
        vertex(x1, y1, color, u1, v1);

        x += font.kernBias * s + ch->xadvance * s;
    }
}

void Imm::useTexture(Texture texture) {
    if (tex.handle != texture.handle) {
        flush();
        tex = texture;
    }
}

void Imm::drawImage(Texture texture, float x, float y) {
    useTexture(texture);
    float x0 = x, y0 = y, x1 = x + tex.width, y1 = y + tex.height;
    float u0 = 0, v1 = 0, u1 = tex.width, v0 = tex.height;
    Color color = { 255, 255, 255, 255 };
    check(6);
    incrementDepth();
    vertex(x0, y0, color, u0, v0);
    vertex(x0, y1, color, u0, v1);
    vertex(x1, y0, color, u1, v0);
    vertex(x0, y1, color, u0, v1);
    vertex(x1, y0, color, u1, v0);
    vertex(x1, y1, color, u1, v1);
}

//estimates the number of segments per octant needed to draw a circle
//at a given radius that appears to be completely smooth
int Imm::circleDetail(float radius) {
    //TODO: use matrix scale to determine actual on-screen size
    return fminf(11, sqrtf(radius / 4)) + 1;
}

int Imm::circleDetail(float radius, float delta) {
    //TODO: use matrix scale to determine actual on-screen size
    return fminf(11, sqrtf(radius / 4)) / QUARTER_PI * fabsf(delta) + 1;
}

void Imm::arc(float x, float y, float dx1, float dy1, float dx2, float dy2) {
    Vec2 a = vec2(dx1, dy1), b = vec2(dx2, dy2);
    float theta = atan2f(cross(a, b), dot(a, b));
    int segments = circleDetail(len(dx1, dy1), theta);
    float px = x + dx1, py = y + dy1;

    beginLine();
    lineVertex(px, py);
    if (segments > 1) {
        float c = cosf(theta / segments);
        float s = sinf(theta / segments);
        Mat2 rot = { c, -s, s, c };
        for (int i = 1; i < segments; ++i) {
            a = rot * a;
            float nx = x + a.x;
            float ny = y + a.y;
            lineVertex(nx, ny);
            px = nx;
            py = ny;
        }
    }
    lineVertex(x + dx2, y + dy2);
    endLine(false);
}

void Imm::arcJoin(float x, float y, float dx1, float dy1, float dx2, float dy2) {
    Vec2 a = vec2(dx1, dy1), b = vec2(dx2, dy2);
    float theta = atan2f(cross(a, b), dot(a, b));
    int segments = circleDetail(r, theta);
    float px = x + dx1, py = y + dy1;
    if (segments > 1) {
        float c = cosf(theta / segments);
        float s = sinf(theta / segments);
        Mat2 rot = { c, -s, s, c };
        for (int i = 1; i < segments; ++i) {
            a = rot * a;
            float nx = x + a.x;
            float ny = y + a.y;
            triangle(x, y, px, py, nx, ny, lineColor);
            px = nx;
            py = ny;
        }
    }
    triangle(x, y, px, py, x + dx2, y + dy2, lineColor);
}

void Imm::beginLine() {
    lineVertexCount = 0;
    incrementDepth();
}

void Imm::lineVertex(float x, float y) {
    //disallow adding consecutive duplicate totalVerts,
    //as it is pointless and just creates an extra edge case
    if (lineVertexCount > 0 && x == lx && y == ly) {
        return;
    }

    if (lineVertexCount == 0) {
        fx = x;
        fy = y;
    } else if (lineVertexCount == 1) {
        sx = x;
        sy = y;
    } else {
        Vec2 p = vec2(px, py);
        Vec2 l = vec2(lx, ly);
        Vec2 v = vec2(x, y);

        float cosPiOver15 = 0.97815f;
        Vec2 leg1 = nor(l - p);
        Vec2 leg2 = nor(v - l);
        if (join == BEVEL || join == ROUND || dot(leg1, -leg2) > cosPiOver15 || dot(leg1, -leg2) < -0.999) {
            float tx =  leg1.y * r;
            float ty = -leg1.x * r;

            if (lineVertexCount == 2) {
                sdx = tx;
                sdy = ty;
            } else {
                triangle(px - pdx, py - pdy, px + pdx, py + pdy, lx - tx, ly - ty, lineColor);
                triangle(px + pdx, py + pdy, lx - tx, ly - ty, lx + tx, ly + ty, lineColor);
            }

            float nx =  leg2.y * r;
            float ny = -leg2.x * r;

            if (join == ROUND) {
                if (cross(leg1, leg2) > 0) {
                    arcJoin(lx, ly, tx, ty, nx, ny);
                } else {
                    arcJoin(lx, ly, -tx, -ty, -nx, -ny);
                }
            } else if (cross(leg1, leg2) > 0) {
                triangle(lx, ly, lx + tx, ly + ty, lx + nx, ly + ny, lineColor);
            } else {
                triangle(lx, ly, lx - tx, ly - ty, lx - nx, ly - ny, lineColor);
            }

            pdx = nx;
            pdy = ny;
        } else {
            Vec2 a = leg2 - leg1;
            Vec2 b = vec2(leg1.y, -leg1.x);
            Vec2 c = a * (r / dot(a, b));

            float bx = c.x, by = c.y;

            if (lineVertexCount == 2) {
                sdx = bx;
                sdy = by;
            } else {
                triangle(px - pdx, py - pdy, px + pdx, py + pdy, lx - bx, ly - by, lineColor);
                triangle(px + pdx, py + pdy, lx - bx, ly - by, lx + bx, ly + by, lineColor);
            }

            pdx = bx;
            pdy = by;
        }
    }

    px = lx;
    py = ly;
    lx = x;
    ly = y;

    lineVertexCount += 1;
}

void Imm::lineCap(float x, float y, float dx, float dy) {
    int segments = circleDetail(r, HALF_PI);
    Vec2 p = vec2(dy, -dx);
    if (segments > 1) {
        float c = cosf(HALF_PI / segments);
        float s = sinf(HALF_PI / segments);
        Mat2 rot = { c, -s, s, c };
        for (int i = 1; i < segments; ++i) {
            Vec2 n = rot * p;
            triangle(x, y, x + p.x, y + p.y, x + n.x, y + n.y, lineColor);
            triangle(x, y, x - p.y, y + p.x, x - n.y, y + n.x, lineColor);
            p = n;
        }
    }
    triangle(x, y, x + p.x, y + p.y, x + dx, y + dy, lineColor);
    triangle(x, y, x - p.y, y + p.x, x - dy, y + dx, lineColor);
}

void Imm::endLine(bool close) {
    if (lineVertexCount < 2) {
        return;
    }

    if (lineVertexCount < 3) {
        line(px, py, lx, ly, lineColor);
    }

    if (close) {
        //draw the last two legs
        lineVertex(fx, fy);
        lineVertex(sx, sy);

        //connect first and second vertices
        triangle(px - pdx, py - pdy, px + pdx, py + pdy, sx - sdx, sy - sdy, lineColor);
        triangle(px + pdx, py + pdy, sx - sdx, sy - sdy, sx + sdx, sy + sdy, lineColor);
    } else {
        //draw last line (with cap)
        float dx = lx - px;
        float dy = ly - py;
        float d = sqrtf(dx*dx + dy*dy);
        float tx =  dy / d * r;
        float ty = -dx / d * r;

        if (cap == PROJECT) {
            lx -= ty;
            ly += tx;
        }

        triangle(px - pdx, py - pdy, px + pdx, py + pdy, lx - tx, ly - ty, lineColor);
        triangle(px + pdx, py + pdy, lx - tx, ly - ty, lx + tx, ly + ty, lineColor);

        if (cap == ROUND) {
            lineCap(lx, ly, -ty, tx);
        }

        //draw first line (with cap)
        dx = fx - sx;
        dy = fy - sy;
        d = sqrtf(dx*dx + dy*dy);
        tx =  dy / d * r;
        ty = -dx / d * r;

        if (cap == PROJECT) {
            fx -= ty;
            fy += tx;
        }

        triangle(sx - sdx, sy - sdy, sx + sdx, sy + sdy, fx + tx, fy + ty, lineColor);
        triangle(sx + sdx, sy + sdy, fx + tx, fy + ty, fx - tx, fy - ty, lineColor);

        if (cap == ROUND) {
            lineCap(fx, fy, -ty, tx);
        }
    }
}

void Imm::line(float x1, float y1, float x2, float y2, Color stroke) {
    incrementDepth();

    float dx = x2 - x1;
    float dy = y2 - y1;
    float d = sqrtf(dx * dx + dy * dy);
    float tx = dy / d * r;
    float ty = dx / d * r;

    if (cap == PROJECT) {
        x1 -= ty;
        x2 += ty;
        y1 -= tx;
        y2 += tx;
    }

    triangle(x1 - tx, y1 + ty, x1 + tx, y1 - ty, x2 - tx, y2 + ty, stroke);
    triangle(x2 + tx, y2 - ty, x2 - tx, y2 + ty, x1 + tx, y1 - ty, stroke);

    if (cap == ROUND) {
        int segments = circleDetail(r, HALF_PI);
        float step = HALF_PI / segments;
        float c = cosf(step);
        float s = sinf(step);
        for (int i = 0; i < segments; ++i) {
            //this is the equivalent of multiplying the vector <tx, ty> by the 2x2 rotation matrix [[c -s] [s c]]
            float nx = c * tx - s * ty;
            float ny = s * tx + c * ty;
            triangle(x2, y2, x2 + ty, y2 + tx, x2 + ny, y2 + nx, stroke);
            triangle(x2, y2, x2 - tx, y2 + ty, x2 - nx, y2 + ny, stroke);
            triangle(x1, y1, x1 - ty, y1 - tx, x1 - ny, y1 - nx, stroke);
            triangle(x1, y1, x1 + tx, y1 - ty, x1 + nx, y1 - ny, stroke);
            tx = nx;
            ty = ny;
        }
    }
}

void Imm::rect(float x, float y, float w, float h, Color c) {
    incrementDepth();

    triangle(x, y, x + w, y, x + w, y + h, c);
    triangle(x, y, x, y + h, x + w, y + h, c);
}

//TODO: optimize this!
void Imm::circle(float x, float y, float radius, Color fill, Color stroke) {
    if (fill.a) {
        incrementDepth();

        int segments = circleDetail(radius);
        float step = QUARTER_PI / segments;

        float x1 = 0;
        float y1 = radius;
        float angle = 0;
        for (int i = 0; i < segments; ++i) {
            angle += step;
            float x2, y2;
            //this is not just for performance
            //it also ensures the circle is drawn with no diagonal gaps
            if (i < segments - 1) {
                x2 = sinf(angle) * radius;
                y2 = cosf(angle) * radius;
            } else {
                x2 = y2 = sinf(QUARTER_PI) * radius;
            }

            triangle(x, y, x + x1, y + y1, x + x2, y + y2, fill);
            triangle(x, y, x + x1, y - y1, x + x2, y - y2, fill);
            triangle(x, y, x - x1, y + y1, x - x2, y + y2, fill);
            triangle(x, y, x - x1, y - y1, x - x2, y - y2, fill);

            triangle(x, y, x + y1, y + x1, x + y2, y + x2, fill);
            triangle(x, y, x + y1, y - x1, x + y2, y - x2, fill);
            triangle(x, y, x - y1, y + x1, x - y2, y + x2, fill);
            triangle(x, y, x - y1, y - x1, x - y2, y - x2, fill);

            x1 = x2;
            y1 = y2;
        }
    }

    if (stroke.a) {
        incrementDepth();

        float r1 = radius - r;
        float r2 = radius + r;

        int segments = circleDetail(r2);
        float step = QUARTER_PI / segments;

        float s1 = 0; //sin
        float c1 = 1; //cos
        float angle = 0;
        for (int i = 0; i < segments; ++i) {
            angle += step;
            float s2, c2;
            //this is not just for performance
            //it also ensures the circle is drawn with no diagonal gaps
            if (i < segments - 1) {
                s2 = sinf(angle);
                c2 = cosf(angle);
            } else {
                s2 = c2 = sinf(QUARTER_PI);
            }

            triangle(x + s1*r1, y + c1*r1, x + s2*r1, y + c2*r1, x + s1*r2, y + c1*r2, stroke);
            triangle(x + s2*r1, y + c2*r1, x + s1*r2, y + c1*r2, x + s2*r2, y + c2*r2, stroke);

            triangle(x + s1*r1, y - c1*r1, x + s2*r1, y - c2*r1, x + s1*r2, y - c1*r2, stroke);
            triangle(x + s2*r1, y - c2*r1, x + s1*r2, y - c1*r2, x + s2*r2, y - c2*r2, stroke);

            triangle(x - s1*r1, y + c1*r1, x - s2*r1, y + c2*r1, x - s1*r2, y + c1*r2, stroke);
            triangle(x - s2*r1, y + c2*r1, x - s1*r2, y + c1*r2, x - s2*r2, y + c2*r2, stroke);

            triangle(x - s1*r1, y - c1*r1, x - s2*r1, y - c2*r1, x - s1*r2, y - c1*r2, stroke);
            triangle(x - s2*r1, y - c2*r1, x - s1*r2, y - c1*r2, x - s2*r2, y - c2*r2, stroke);

            triangle(x + c1*r1, y + s1*r1, x + c2*r1, y + s2*r1, x + c1*r2, y + s1*r2, stroke);
            triangle(x + c2*r1, y + s2*r1, x + c1*r2, y + s1*r2, x + c2*r2, y + s2*r2, stroke);

            triangle(x + c1*r1, y - s1*r1, x + c2*r1, y - s2*r1, x + c1*r2, y - s1*r2, stroke);
            triangle(x + c2*r1, y - s2*r1, x + c1*r2, y - s1*r2, x + c2*r2, y - s2*r2, stroke);

            triangle(x - c1*r1, y + s1*r1, x - c2*r1, y + s2*r1, x - c1*r2, y + s1*r2, stroke);
            triangle(x - c2*r1, y + s2*r1, x - c1*r2, y + s1*r2, x - c2*r2, y + s2*r2, stroke);

            triangle(x - c1*r1, y - s1*r1, x - c2*r1, y - s2*r1, x - c1*r2, y - s1*r2, stroke);
            triangle(x - c2*r1, y - s2*r1, x - c1*r2, y - s1*r2, x - c2*r2, y - s2*r2, stroke);

            s1 = s2;
            c1 = c2;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
///
////////////////////////////////////////////////////////////////////////////////

float text_width(Font font, float scale, const char * text) {
    return text_width(font, scale, text, strlen(text));
}

float text_width(Font font, float scale, const char * text, int len) {
    float width = 0;
    for (int i = 0; i < len; ++i) {
        Char * ch = font.chars + text[i];
        width += font.kernBias * scale + ch->xadvance * scale;
    }
    return width;
}
