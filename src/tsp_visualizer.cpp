/*
 * ============================================================
 *  Travelling Salesman Problem Visualizer
 *  Language : C++17  |  Graphics : SFML 3.1.0
 *  Algorithms: Nearest Neighbour + 2-opt
 *
 *  COMPILE:
 *  g++ -std=c++17 tsp_visualizer.cpp -o tsp -IC:\SFML-3.1.0\include -LC:\SFML-3.1.0\lib -lsfml-graphics -lsfml-window -lsfml-system -mwindows
 *
 *  CONTROLS:
 *    Left Click -> Add city   |   ENTER -> Nearest Neighbour
 *    O -> 2-opt improve       |   R -> Reset   |   ESC -> Quit
 * ============================================================
 */

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <iomanip>

const int   WIN_W    = 1200;
const int   WIN_H    = 720;
const int   PANEL_W  = 300;
const int   CANVAS_W = WIN_W - PANEL_W;
const float CITY_R   = 9.f;

// Palette — clean, visible, not flashy
const sf::Color C_BG       {14,  15,  25};
const sf::Color C_PANEL    {20,  22,  38};
const sf::Color C_PANEL2   {26,  28,  48};
const sf::Color C_BORDER   {50,  55,  90};
const sf::Color C_BORDER2  {70,  75, 120};
const sf::Color C_CITY     {0,   220, 150};
const sf::Color C_ROUTE    {140, 120, 255};
const sf::Color C_OPT      {0,   220, 150};
const sf::Color C_TEXT     {220, 222, 240};
const sf::Color C_TEXT2    {160, 165, 200};
const sf::Color C_MUTED    {100, 105, 145};
const sf::Color C_ACCENT   {255, 120,  70};
const sf::Color C_START    {255, 210,  60};
const sf::Color C_PURPLE   {190, 175, 255};
const sf::Color C_GREEN    {0,   220, 150};
const sf::Color C_HINT     {130, 135, 180};  // visible hint text

struct City { float x, y; };

float edgeDist(const City& a, const City& b){
    float dx=a.x-b.x, dy=a.y-b.y;
    return std::sqrt(dx*dx+dy*dy);
}
float tourLen(const std::vector<int>& t, const std::vector<City>& c){
    float s=0; int n=(int)t.size();
    for(int i=0;i<n;++i) s+=edgeDist(c[t[i]],c[t[(i+1)%n]]);
    return s;
}

std::vector<int> nearestNeighbour(const std::vector<City>& cities){
    int n=(int)cities.size();
    std::vector<bool> vis(n,false);
    std::vector<int> tour; tour.reserve(n);
    int cur=0; vis[cur]=true; tour.push_back(cur);
    for(int s=1;s<n;++s){
        float best=std::numeric_limits<float>::max(); int next=-1;
        for(int j=0;j<n;++j) if(!vis[j]){float d=edgeDist(cities[cur],cities[j]);if(d<best){best=d;next=j;}}
        vis[next]=true; tour.push_back(next); cur=next;
    }
    return tour;
}

std::vector<int> twoOpt(std::vector<int> tour, const std::vector<City>& cities){
    int n=(int)tour.size(); bool improved=true;
    while(improved){
        improved=false;
        for(int i=0;i<n-1;++i) for(int j=i+2;j<n;++j){
            if(i==0&&j==n-1) continue;
            float before=edgeDist(cities[tour[i]],cities[tour[i+1]])+edgeDist(cities[tour[j]],cities[tour[(j+1)%n]]);
            float after =edgeDist(cities[tour[i]],cities[tour[j]])+edgeDist(cities[tour[i+1]],cities[tour[(j+1)%n]]);
            if(after<before-1e-4f){std::reverse(tour.begin()+i+1,tour.begin()+j+1);improved=true;}
        }
    }
    return tour;
}

// ── Draw helpers ─────────────────────────────────────────────
void drawRect(sf::RenderWindow& w, float x, float y, float ww, float hh,
              sf::Color fill, sf::Color out=sf::Color::Transparent, float th=0){
    sf::RectangleShape r({ww,hh}); r.setPosition({x,y});
    r.setFillColor(fill); r.setOutlineColor(out); r.setOutlineThickness(th);
    w.draw(r);
}
void drawCircle(sf::RenderWindow& w, float x, float y, float r, sf::Color fill){
    sf::CircleShape c(r); c.setOrigin({r,r}); c.setPosition({x,y}); c.setFillColor(fill); w.draw(c);
}
void drawLine(sf::RenderWindow& w, float x1, float y1, float x2, float y2, sf::Color col, float thickness=1.5f){
    // draw as thin rectangle for thickness support
    float dx=x2-x1, dy=y2-y1;
    float len=std::sqrt(dx*dx+dy*dy);
    if(len<0.01f) return;
    sf::RectangleShape line({len, thickness});
    line.setPosition({x1,y1});
    line.setFillColor(col);
    line.setRotation(sf::radians(std::atan2(dy,dx)));
    line.setOrigin({0, thickness/2.f});
    w.draw(line);
}
void drawText(sf::RenderWindow& w, sf::Font& f, const std::string& s,
              float x, float y, unsigned sz, sf::Color col, bool bold=false){
    sf::Text t(f,s,sz); t.setPosition({x,y}); t.setFillColor(col);
    if(bold) t.setStyle(sf::Text::Style::Bold);
    w.draw(t);
}
void drawTextCentered(sf::RenderWindow& w, sf::Font& f, const std::string& s,
                      float cx, float y, unsigned sz, sf::Color col, bool bold=false){
    sf::Text t(f,s,sz);
    if(bold) t.setStyle(sf::Text::Style::Bold);
    auto b=t.getLocalBounds();
    t.setPosition({cx - b.size.x/2.f, y});
    t.setFillColor(col);
    w.draw(t);
}

// ── App state ────────────────────────────────────────────────
enum class State { IDLE, NN_DONE, OPT_DONE };
struct App {
    std::vector<City> cities;
    std::vector<int>  tour;
    State state=State::IDLE;
    float nnDist=0, optDist=0;
    std::string status="Click on the canvas to place cities";
    sf::Color statusCol=C_HINT;
};

// ── Canvas ───────────────────────────────────────────────────
void drawCanvas(sf::RenderWindow& win, App& app, sf::Font& font){
    drawRect(win,0,0,CANVAS_W,WIN_H,C_BG);

    // subtle dot grid
    for(int gx=40;gx<CANVAS_W;gx+=40)
        for(int gy=40;gy<WIN_H;gy+=40)
            drawCircle(win,(float)gx,(float)gy,1.f,{40,43,70});

    // canvas border right edge
    drawRect(win,CANVAS_W-1,0,1,WIN_H,C_BORDER);

    // Empty state hint — clearly visible
    if(app.cities.empty()){
        float cx = CANVAS_W / 2.f;
        float cy = WIN_H / 2.f;

        // centre box
        drawRect(win, cx-220, cy-80, 440, 160, {20,22,40}, C_BORDER, 1.f);

        drawTextCentered(win, font, "TSP Visualizer", cx, cy-68, 22, C_TEXT, true);
        drawTextCentered(win, font, "Click anywhere on the canvas to place cities", cx, cy-38, 14, C_HINT);

        // key hints
        struct KH { std::string key, desc; };
        std::vector<KH> khs = {{"ENTER","Run Nearest Neighbour"},{"O","Apply 2-opt Improvement"},{"R","Reset everything"}};
        float ky = cy + 4.f;
        for(auto& k : khs){
            // key badge
            drawRect(win, cx-180, ky, 52, 20, {35,38,65}, C_BORDER2, 1.f);
            drawTextCentered(win, font, k.key, cx-180+26, ky+3, 12, C_PURPLE, true);
            drawText(win, font, k.desc, cx-118, ky+3, 13, C_HINT);
            ky += 26.f;
        }
        return;
    }

    // Draw tour edges
    if(!app.tour.empty()){
        sf::Color lc=(app.state==State::OPT_DONE)?C_OPT:C_ROUTE;
        int n=(int)app.tour.size();
        for(int i=0;i<n;++i){
            auto& a=app.cities[app.tour[i]];
            auto& b=app.cities[app.tour[(i+1)%n]];
            drawLine(win,a.x,a.y,b.x,b.y,{lc.r,lc.g,lc.b,180},2.f);
        }
    }

    // Draw cities
    for(int i=0;i<(int)app.cities.size();++i){
        auto& c=app.cities[i];
        sf::Color col=(i==0&&!app.tour.empty())?C_START:C_CITY;
        // outer glow
        drawCircle(win,c.x,c.y,CITY_R+7.f,{col.r,col.g,col.b,30});
        drawCircle(win,c.x,c.y,CITY_R+3.f,{col.r,col.g,col.b,60});
        // main dot
        drawCircle(win,c.x,c.y,CITY_R,col);
        // inner highlight
        drawCircle(win,c.x-2.f,c.y-2.f,3.f,{255,255,255,60});
        // label — offset above and right, clearly visible
        drawText(win,font,std::to_string(i+1),c.x+CITY_R+5,c.y-CITY_R-4,13,C_TEXT,true);
    }

    // If tour exists, show start arrow label
    if(!app.tour.empty()){
        auto& s=app.cities[app.tour[0]];
        drawText(win,font,"START",s.x+CITY_R+5,s.y+CITY_R+2,11,C_START,true);
    }
}

// ── Panel ────────────────────────────────────────────────────
void drawPanel(sf::RenderWindow& win, App& app, sf::Font& font){
    float px=(float)CANVAS_W, pw=(float)PANEL_W;

    // Panel bg with subtle gradient via two rects
    drawRect(win,px,0,pw,WIN_H,C_PANEL);
    drawRect(win,px,0,pw,4,C_CITY); // top accent bar

    float y=16.f;

    // Title block
    drawText(win,font,"TSP VISUALIZER",px+16,y,16,C_CITY,true); y+=22.f;
    drawText(win,font,"Travelling Salesman Problem",px+16,y,11,C_MUTED); y+=14.f;
    drawText(win,font,"Design & Analysis of Algorithms",px+16,y,11,C_MUTED); y+=22.f;

    drawRect(win,px+16,y,pw-32,1,C_BORDER); y+=14.f;

    // ── Stats ──
    drawText(win,font,"STATISTICS",px+16,y,10,C_MUTED,true); y+=14.f;

    auto statBox=[&](const std::string& val, const std::string& lbl, const std::string& sub,
                     float bx, float by, sf::Color vc){
        drawRect(win,bx,by,130,58,C_PANEL2,C_BORDER,1.f);
        drawText(win,font,val,bx+10,by+7,20,vc,true);
        drawText(win,font,lbl,bx+10,by+33,11,C_TEXT2,true);
        drawText(win,font,sub,bx+10,by+46,10,C_MUTED);
    };

    statBox(std::to_string(app.cities.size()),"Cities placed","on canvas",px+16,y,C_CITY);

    std::ostringstream ds;
    ds<<std::fixed<<std::setprecision(0)<<app.nnDist;
    statBox(app.nnDist>0?ds.str():"--","NN Distance","pixels (units)",px+154,y,C_PURPLE);
    y+=70.f;

    if(app.optDist>0){
        std::ostringstream os,ps;
        os<<std::fixed<<std::setprecision(0)<<app.optDist;
        float pct=100.f*(app.nnDist-app.optDist)/app.nnDist;
        ps<<std::fixed<<std::setprecision(1)<<pct<<"%";
        statBox(os.str(),"2-opt Distance","after improvement",px+16,y,C_GREEN);
        statBox(ps.str(),"Route Improved","vs NN solution",px+154,y,C_START);
        y+=70.f;
    } else {
        y+=4.f;
    }

    drawRect(win,px+16,y,pw-32,1,C_BORDER); y+=14.f;

    // ── Controls ──
    drawText(win,font,"CONTROLS",px+16,y,10,C_MUTED,true); y+=14.f;

    struct Btn{ std::string key,lbl,desc; sf::Color bg,tc; };
    std::vector<Btn> btns={
        {"ENTER","Nearest Neighbour","Greedy heuristic solver",{0,55,38},C_CITY},
        {"O","2-opt Improve","Edge swap optimiser",{38,28,75},C_PURPLE},
        {"R","Reset Canvas","Clear all cities & routes",{60,22,12},C_ACCENT},
    };
    for(auto& b:btns){
        drawRect(win,px+16,y,pw-32,46,b.bg,C_BORDER,1.f);
        // key badge
        drawRect(win,px+24,y+13,44,20,{0,0,0,80},C_BORDER2,1.f);
        drawTextCentered(win,font,b.key,px+46,y+15,11,C_TEXT,true);
        drawText(win,font,b.lbl,px+76,y+8,14,b.tc,true);
        drawText(win,font,b.desc,px+76,y+26,11,C_MUTED);
        y+=54.f;
    }

    drawRect(win,px+16,y,pw-32,1,C_BORDER); y+=14.f;

    // ── Algorithm info ──
    drawText(win,font,"HOW IT WORKS",px+16,y,10,C_MUTED,true); y+=14.f;

    // NN explanation
    drawRect(win,px+16,y,pw-32,72,C_PANEL2,C_BORDER,1.f);
    drawRect(win,px+16,y,3,72,C_PURPLE); // left accent
    drawText(win,font,"Nearest Neighbour",px+26,y+6,13,C_PURPLE,true);
    drawText(win,font,"Start at city 0. At each step,",px+26,y+24,11,C_TEXT2);
    drawText(win,font,"visit the closest unvisited city.",px+26,y+37,11,C_TEXT2);
    drawText(win,font,"Complexity: O(n^2)",px+26,y+52,11,C_MUTED);
    y+=80.f;

    // 2-opt explanation
    drawRect(win,px+16,y,pw-32,72,C_PANEL2,C_BORDER,1.f);
    drawRect(win,px+16,y,3,72,C_CITY);
    drawText(win,font,"2-opt Improvement",px+26,y+6,13,C_CITY,true);
    drawText(win,font,"Try reversing segments between",px+26,y+24,11,C_TEXT2);
    drawText(win,font,"pairs of edges. Keep if shorter.",px+26,y+37,11,C_TEXT2);
    drawText(win,font,"Complexity: O(n^2) per pass",px+26,y+52,11,C_MUTED);
    y+=80.f;

    drawRect(win,px+16,y,pw-32,1,C_BORDER); y+=12.f;

    // ── Legend ──
    drawText(win,font,"LEGEND",px+16,y,10,C_MUTED,true); y+=14.f;

    struct Leg{ sf::Color col; std::string lbl; bool isLine; };
    std::vector<Leg> legs={
        {C_START,"Start city (city 0)",false},
        {C_CITY, "Regular city node",false},
        {C_ROUTE,"Nearest Neighbour route",true},
        {C_OPT,  "2-opt optimised route",true},
    };
    for(auto& l:legs){
        if(l.isLine) drawRect(win,px+16,y+7,24,3,l.col);
        else         drawCircle(win,px+28,y+8,6.f,l.col);
        drawText(win,font,l.lbl,px+48,y+2,12,C_TEXT2);
        y+=22.f;
    }

    // ── Status bar ──
    float sy=WIN_H-50.f;
    drawRect(win,px,sy-1,pw,1,C_BORDER);
    drawRect(win,px,sy,pw,51,{16,18,32});
    drawRect(win,px,sy,4,51,app.statusCol); // left colour bar

    // word wrap status into 2 lines
    std::string st=app.status;
    if((int)st.size()>30){
        size_t sp=st.rfind(' ',30);
        if(sp!=std::string::npos){
            drawText(win,font,st.substr(0,sp),px+14,sy+8,12,app.statusCol,true);
            drawText(win,font,st.substr(sp+1),px+14,sy+26,12,{app.statusCol.r,app.statusCol.g,app.statusCol.b,200});
        } else drawText(win,font,st,px+14,sy+16,12,app.statusCol,true);
    } else {
        drawText(win,font,st,px+14,sy+16,12,app.statusCol,true);
    }
}

// ── Main ─────────────────────────────────────────────────────
int main(){
    sf::RenderWindow window(
        sf::VideoMode({(unsigned)WIN_W,(unsigned)WIN_H}),
        "TSP Visualizer - DAA Project"
    );
    window.setFramerateLimit(60);

    sf::Font font;
    for(auto& p:{
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
    }) if(font.openFromFile(p)) break;

    App app;

    while(window.isOpen()){
        while(auto ev=window.pollEvent()){
            if(ev->is<sf::Event::Closed>()) window.close();

            if(auto* k=ev->getIf<sf::Event::KeyPressed>()){
                if(k->code==sf::Keyboard::Key::Escape) window.close();

                if(k->code==sf::Keyboard::Key::R){
                    app=App{};
                    app.status="Reset! Click to place cities.";
                    app.statusCol=C_HINT;
                }
                if(k->code==sf::Keyboard::Key::Enter){
                    if(app.cities.size()<2){
                        app.status="Need at least 2 cities first!";
                        app.statusCol=C_ACCENT;
                    } else {
                        app.tour=nearestNeighbour(app.cities);
                        app.nnDist=tourLen(app.tour,app.cities);
                        app.optDist=0;
                        app.state=State::NN_DONE;
                        app.status="NN solved! Press O to optimise.";
                        app.statusCol=C_PURPLE;
                    }
                }
                if(k->code==sf::Keyboard::Key::O){
                    if(app.tour.empty()){
                        app.status="Run Nearest Neighbour first!";
                        app.statusCol=C_ACCENT;
                    } else {
                        app.tour=twoOpt(app.tour,app.cities);
                        app.optDist=tourLen(app.tour,app.cities);
                        app.state=State::OPT_DONE;
                        app.status="2-opt done! Route optimised.";
                        app.statusCol=C_CITY;
                    }
                }
            }

            if(auto* mb=ev->getIf<sf::Event::MouseButtonPressed>()){
                if(mb->button==sf::Mouse::Button::Left){
                    // FIX: use exact mouse position, no offset
                    float mx=(float)mb->position.x;
                    float my=(float)mb->position.y;
                    if(mx>=0 && mx<(float)CANVAS_W && my>=0 && my<(float)WIN_H){
                        app.cities.push_back({mx, my});
                        app.tour.clear();
                        app.state=State::IDLE;
                        app.nnDist=0; app.optDist=0;
                        app.status="City "+std::to_string(app.cities.size())+" added. Press ENTER to solve.";
                        app.statusCol=C_TEXT2;
                    }
                }
            }
        }

        window.clear(C_BG);
        drawCanvas(window,app,font);
        drawPanel(window,app,font);
        window.display();
    }
    return 0;
}
