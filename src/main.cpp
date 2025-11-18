// ==== Detective Eclipse — Obsidian Rose (Linux, Final Visuals) ====
// - DejaVu fonts (Linux)
// - Lives start at 6
// - Stack-based mismatch streak: 2 mismatches => -1 life (toast); 3rd => swap two hidden cards (toast), reset streak
// - No shuffle animation (toast only)
// - Three suspects vertically (Serena / Marcus / Evelyn) with distinct colors; journal on the right
// - Suspect screen background image from user's path
// - Simplified story text
// - Leaderboard: max-heap + heapsort, Top 10 display

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <algorithm>
#include <vector>
#include <random>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <stack>
#include <queue>
#include <cstdlib>

using namespace std;

// ------------------ Paths ------------------
static const char* INTRO_IMAGE_PATH   = "/home/saad/Downloads/closed-notebook-leather-cover-sheet-white-paper-felt-brown-hat-camera-big-magnifier-isolated-black-aged-wood-table.jpg";
static const char* STORY_BG_PATH      = "/home/saad/Downloads/detective23_crash_bg.png";
static const char* SHOTGUN_SFX_PATH   = "/home/saad/Downloads/sound-effects-single-gun-shot-247124.mp3";
static const char* HEART_IMAGE_PATH   = "/home/saad/Downloads/heartbonus.png";
static const char* SHOTGUN_IMAGE_PATH = "/home/saad/Downloads/shotgun.jpeg";
static const char* INTERLUDE_BG_PATH  = "/home/saad/Pictures/Screenshots/Screenshot From 2025-11-12 16-51-25.png";

// Scene background + CARD BACK
static const char* GAME_BG_PATH         = "/home/saad/Downloads/visax-r9DV-EdDmWM-unsplash.jpg";
static const char* CARD_BACK_IMAGE_PATH = "/home/saad/Pictures/Screenshots/Screenshot From 2025-11-15 20-14-06.png";

// Detective journal background
static const char* JOURNAL_BG_IMAGE_PATH = "/home/saad/Pictures/Screenshots/Screenshot From 2025-11-15 19-31-05.png";

// NEW: Suspects screen background (your requested image)
static const char* SUSPECT_BG_PATH = "/home/saad/Pictures/Screenshots/Screenshot From 2025-11-16 17-47-00.png";

static const char* UI_FONT_PATH    = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
static const char* STORY_FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf";

// Game constants
static const int   INITIAL_LIVES = 6;

// ------------------ UTF-8 helpers ------------------
inline void setUtf8(sf::Text& t, const string& s){ t.setString(sf::String::fromUtf8(s.begin(), s.end())); }
inline sf::String utf8(const string& s){ return sf::String::fromUtf8(s.begin(), s.end()); }

// ------------------ Easing ------------------
inline float easeInOutCubic(float t){ return t<0.5f ? 4.f*t*t*t : 1.f - std::pow(-2.f*t+2.f,3.f)/2.f; }

// ------------------ Card ------------------
struct Card {
    sf::RectangleShape rect;
    int value = 0;
    int type  = 0; // 0: normal, 1: heart, 2: shotgun
    bool faceUp   = false;
    bool matched  = false;
    bool flipping = false;
    bool targetUp = false;
    float flipT   = 0.f;
    sf::Color frontColor = sf::Color::White;
    int displayNumber = 0;
};

// ------------------ Leaderboard (heap version) ------------------
struct LeaderRecord { float seconds=0.f; bool won=false; string date; };

inline string nowDate(){
    time_t t=std::time(nullptr); char b[32];
    std::strftime(b,sizeof(b),"%Y-%m-%d %H:%M", std::localtime(&t));
    return b;
}

// Save: seconds;won;date
inline void saveRecord(const LeaderRecord& r, const string& path="leaderboard.csv"){
    ofstream ofs(path, ios::app);
    if(ofs) ofs<<r.seconds<<';'<<(r.won? "1":"0")<<';'<<r.date<<"\n";
}

// Back-compat load
inline vector<LeaderRecord> loadRecords(const string& path="leaderboard.csv"){
    vector<LeaderRecord> v; ifstream ifs(path); string line;
    while(getline(ifs,line)){
        if(line.empty()) continue;
        vector<string> tok; stringstream ss(line); string part;
        while(getline(ss, part, ';')) tok.push_back(part);
        LeaderRecord r;
        try{
            if(tok.size()>=4){ // old: name;seconds;lives;date
                r.seconds = stof(tok[1]); int lives = stoi(tok[2]); r.won = (lives>0); r.date = tok[3]; v.push_back(r);
            } else if(tok.size()>=3){ // new: seconds;won;date
                r.seconds = stof(tok[0]); r.won = (tok[1]=="1"||tok[1]=="true"||tok[1]=="True"); r.date = tok[2]; v.push_back(r);
            } else if(tok.size()==2){ // seconds;won
                r.seconds = stof(tok[0]); r.won = (tok[1]=="1"||tok[1]=="true"||tok[1]=="True"); r.date = ""; v.push_back(r);
            }
        } catch(...) {}
    }
    return v;
}

// ------------------ Drawing helpers ------------------
void drawGradient(sf::RenderWindow& win, sf::Color top, sf::Color bottom){
    sf::VertexArray q(sf::TriangleStrip,4);
    q[0].position={0,0}; q[1].position={(float)win.getSize().x,0};
    q[2].position={0,(float)win.getSize().y}; q[3].position={(float)win.getSize().x,(float)win.getSize().y};
    q[0].color=q[1].color=top; q[2].color=q[3].color=bottom; win.draw(q);
}
void drawCentered(sf::RenderWindow& win, sf::Text t, float y){
    auto b=t.getLocalBounds(); t.setOrigin(b.left+b.width/2.f, b.top+b.height/2.f);
    t.setPosition(win.getSize().x/2.f, y); win.draw(t);
}
void drawVignette(sf::RenderWindow& win, int edge=64, sf::Uint8 alpha=90){
    sf::Vector2f sz((float)win.getSize().x,(float)win.getSize().y);
    sf::RectangleShape top({sz.x,(float)edge}), bot({sz.x,(float)edge}), left({(float)edge,sz.y}), right({(float)edge,sz.y});
    top.setPosition(0,0); bot.setPosition(0,sz.y-edge); left.setPosition(0,0); right.setPosition(sz.x-edge,0);
    sf::Color c(0,0,0,alpha);
    top.setFillColor(c); bot.setFillColor(c); left.setFillColor(c); right.setFillColor(c);
    win.draw(top); win.draw(bot); win.draw(left); win.draw(right);
}
void drawGrain(sf::RenderWindow& win, mt19937& rng, int dots=100){
    uniform_int_distribution<int> x(0,(int)win.getSize().x-1), y(0,(int)win.getSize().y-1);
    for(int i=0;i<dots;i++){
        sf::Vertex v[] = { sf::Vertex(sf::Vector2f((float)x(rng),(float)y(rng)), sf::Color(255,255,255,12)) };
        win.draw(v, 1, sf::Points);
    }
}

// ------------------ Toast queue (FIFO) ------------------
struct Toast{ string msg; float t; };
queue<Toast> toastQ;

void pushToast(const string& m, float dur=1.2f){ toastQ.push({m,dur}); }

void updateAndDrawToasts(sf::RenderWindow& win, sf::Font& uiFont, float dt){
    if(toastQ.empty()) return;
    Toast& cur = toastQ.front();
    cur.t -= dt;
    sf::Text tx("", uiFont, 22); setUtf8(tx, cur.msg);
    tx.setFillColor(sf::Color(255,255,200));
    tx.setOutlineThickness(2.f);
    tx.setOutlineColor(sf::Color(0,0,0,150));
    drawCentered(win, tx, 46.f);
    if(cur.t <= 0.f) toastQ.pop();
}

// ------------------ Journal (wrapped & clipped, with photo BG) ------------------
struct ScrollState { float scroll=0.f; float contentH=0.f; };

void drawJournalPanel(sf::RenderWindow& win, sf::Font& font,
                      sf::Vector2f pos, sf::Vector2f size,
                      const vector<string>& entries,
                      ScrollState& st,
                      unsigned fontSize,
                      const char* bgTexPath)
{
    // soft shadow
    sf::RectangleShape shadow({size.x+10.f, size.y+10.f});
    shadow.setPosition(pos.x+5.f, pos.y+6.f);
    shadow.setFillColor(sf::Color(0,0,0,110));
    win.draw(shadow);

    // panel body
    sf::RectangleShape panel(size);
    panel.setPosition(pos);
    panel.setFillColor(sf::Color(22, 28, 56, 240));
    panel.setOutlineThickness(2.f);
    panel.setOutlineColor(sf::Color(210, 210, 230));
    win.draw(panel);

    // header
    sf::RectangleShape header({size.x, 38.f});
    header.setPosition(pos);
    header.setFillColor(sf::Color(35, 44, 86));
    win.draw(header);

    sf::Text title("",font,21);
    title.setFillColor(sf::Color(255, 235, 150));
    title.setPosition(pos.x + 12.f, pos.y + 6.f);
    setUtf8(title, "Detective's Journal");
    win.draw(title);

    // render area
    sf::RenderTexture rt; rt.create((unsigned)(size.x - 24.f), (unsigned)(size.y - 56.f));
    float innerX = pos.x + 12.f;
    float innerY = pos.y + 44.f;

    // BG IMAGE inside the content area
    static sf::Texture bgTex; static bool bgLoaded=false; static string lastPath;
    if(bgTexPath && (!bgLoaded || lastPath!=bgTexPath)){
        bgLoaded = bgTex.loadFromFile(bgTexPath);
        lastPath = bgTexPath? bgTexPath : "";
    }

    // text + wrap
    sf::Text meas("",font,fontSize), text("",font,fontSize);
    text.setFillColor(sf::Color(220, 235, 255));

    auto wrap = [&](const string& src, float maxW){
        vector<string> lines; istringstream iss(src); string w,line;
        while(iss>>w){
            string trial = line.empty()? w : line + " " + w;
            setUtf8(meas, trial);
            if(meas.getLocalBounds().width > maxW){
                if(!line.empty()) lines.push_back(line);
                line = w;
            } else line = trial;
        }
        if(!line.empty()) lines.push_back(line);
        if(lines.empty()) lines.push_back("");
        return lines;
    };

    ostringstream out;
    for(size_t i=0;i<entries.size();++i){
        string prefix = to_string((int)i+1) + ") ";
        float pfw = sf::Text(prefix,font,fontSize).getLocalBounds().width;
        auto lines = wrap(entries[i], rt.getSize().x - pfw);
        out<<prefix<<lines[0]<<"\n";
        // indent following wrapped lines
        string indent((size_t)ceil(pfw/7.f), ' ');
        for(size_t k=1;k<lines.size();++k) out<<indent<<lines[k]<<"\n";
        if((i+1)%3==0) out<<"------------------------------------------------\n";
    }

    setUtf8(text, out.str());
    st.contentH = text.getGlobalBounds().height + 6.f;
    float viewH  = size.y - 56.f;
    st.scroll = clamp(st.scroll, 0.f, max(0.f, st.contentH - viewH));

    // draw content area with BG
    rt.clear(sf::Color::Transparent);
    if(bgLoaded){
        sf::Sprite bg(bgTex);
        float sx = rt.getSize().x  / (float)bgTex.getSize().x;
        float sy = rt.getSize().y  / (float)bgTex.getSize().y;
        float sc = max(sx, sy);
        bg.setScale(sc, sc);
        auto b = bg.getLocalBounds();
        bg.setOrigin(b.width/2.f, b.height/2.f);
        bg.setPosition(rt.getSize().x/2.f, rt.getSize().y/2.f);
        rt.draw(bg);
        // readability veil
        sf::RectangleShape veil({(float)rt.getSize().x,(float)rt.getSize().y});
        veil.setFillColor(sf::Color(0,0,0,80));
        rt.draw(veil);
    }
    text.setPosition(0.f, -st.scroll);
    rt.draw(text);
    rt.display();

    sf::Sprite s(rt.getTexture());
    s.setPosition(innerX, innerY);
    win.draw(s);

    // scrollbar
    if (st.contentH > viewH + 1.f){
        float barH = max(20.f, viewH * (viewH / st.contentH));
        float barY = innerY + (viewH - barH) * (st.scroll / (st.contentH - viewH));
        sf::RectangleShape bar({4.f, barH});
        bar.setPosition(pos.x + size.x - 8.f, barY);
        bar.setFillColor(sf::Color(180,190,220,200));
        win.draw(bar);
    }
}

// ------------------ Story & bios (simplified) ------------------
static const char* PROLOGUE_TEXT =
"OBSIDIAN ROSE\n\n"
"Elias Vance, the tech billionaire behind Chronos Tech, was found dead in his locked penthouse. "
"On his chest lay a single black rose made of obsidian.\n\n"
"Detective Alistair Knight had solved the case—he knew the name. "
"On his way to announce it, headlights flashed, metal screamed, and everything went dark. "
"He woke up alive, but his memory of the killer was gone.\n\n"
"You will recover his memory by matching pairs. Each match reveals a clue in your Journal. "
"When you have enough, choose the killer.";

// CAST / BIOS
struct Bio { string name, role, note; };
vector<Bio> bios(){
    return {
        {"Alistair Knight", "Detective", "Survived a crash. Memories scattered. Your job is to put them back together."},
        {"Elias Vance", "Victim", "Billionaire founder of Chronos Tech. Found with an obsidian rose on his chest."},
        {"Dr. Serena Bellwether", "Ex Co-founder", "Forced out before IPO. Works with rare plants. Calm face, cold anger."},
        {"Marcus Thorne", "Adopted Son", "Cut from the will. Debts, a temper, and a heavy ring with a sharp diamond."},
        {"Evelyn Reed", "Executive Assistant", "Loyal for years. Knew everything. Wrote a resignation she never sent."}
    };
}

// 13 memory fragments (split across levels)
vector<string> clues_all(){
    return {
        "A burnt stock printout shows a huge leveraged short against Chronos Tech, 48 hours before the murder.",
        "A vintage silver lighter engraved 'M.T.' was found hidden in the study's lounge.",
        "Rare greenhouse soil was stuck to Vance’s shoe—used only in controlled labs.",
        "Security entry shows the study lock was disabled for exactly 30 seconds at 2:03 AM.",
        "An almost-empty anti-anxiety vial registered to Dr. Serena was in the outside bin.",
        "A secret patent describes a pocket EMP that knocks out cameras and mics for seconds.",
        "A broken voice note: 'You promised me... I loved you...' (identity muffled).",
        "Evelyn’s unsent resignation cites 'unbearable pressure and moral conflict.'",
        "The obsidian rose base has a tiny scratch that matches Marcus’s diamond signet.",
        "A receipt shows Marcus cashed out of poker at 12:45 AM—earlier than he claimed.",
        "An old photo shows Marcus and Elias in front of the same greenhouse Serena now runs.",
        "Toxicology: the poison was a custom plant compound Serena specializes in.",
        "Knight’s own note from the crash: 'The motive is not HATE... but FEAR.'"
    };
}
vector<string> clues_level1(){
    auto all = clues_all(); return vector<string>(all.begin(), all.begin()+6);
}
vector<string> clues_level2(){
    auto all = clues_all(); return vector<string>(all.begin()+6, all.end());
}

// ------------------ Audio (gunshot only) ------------------
struct Audio {
    sf::SoundBuffer shotgunBuf;
    sf::Sound shotgun;
} audio;

// ------------------ Stars (prologue CAST background) ------------------
struct Star { sf::Vector2f p; sf::Vector2f v; float r; float a; };
void drawStars(sf::RenderWindow& win, vector<Star>& stars){
    const float W = (float)win.getSize().x, H=(float)win.getSize().y;
    for (auto& s : stars) {
        s.p += s.v;
        if (s.p.x < 0) s.p.x += W; if (s.p.x > W) s.p.x -= W;
        if (s.p.y < 0) s.p.y += H; if (s.p.y > H) s.p.y -= H;
        s.a = 150.f + 100.f * std::sin((s.p.x + s.p.y) * 0.01f);
        sf::CircleShape c(s.r); c.setOrigin(s.r,s.r); c.setPosition(s.p);
        sf::Uint8 alpha = (sf::Uint8)clamp((int)s.a,0,255);
        c.setFillColor(sf::Color(255,255,255,alpha)); win.draw(c);
    }
}

// Hover glow pulse
float glowT=0.f;
sf::Color pulse(float t){
    float a = 0.5f + 0.5f*std::sin(t*3.2f);
    return sf::Color(120 + (int)(60*a), 140 + (int)(40*a), 220, 255);
}

// ------------------ Utility ------------------
// Swap two hidden (unmatched & face-down) cards—can be normal or special.
void swapTwoHidden(vector<Card>& cards, mt19937& rng){
    vector<int> idx;
    for(int i=0;i<(int)cards.size();++i)
        if(!cards[i].matched && !cards[i].faceUp) idx.push_back(i);
    if(idx.size()<2) return;
    shuffle(idx.begin(), idx.end(), rng);
    int a = idx[0], b = idx[1];
    auto pa=cards[a].rect.getPosition(), pb=cards[b].rect.getPosition();
    cards[a].rect.setPosition(pb); cards[b].rect.setPosition(pa);
}

// Build deck: pairs of normals + hearts + shotguns
vector<pair<int,int>> buildDeck(int rows,int cols,int hearts,int shotguns){
    int total=rows*cols;
    int specials=hearts+shotguns;
    int normals = max(0, total - specials);
    if (normals%2==1){ normals--; hearts++; } // keep pairs even
    int pairs = normals/2;
    vector<pair<int,int>> deck;
    for(int v=0; v<pairs; ++v){ deck.push_back({0,v}); deck.push_back({0,v}); }
    for(int i=0;i<hearts;++i)   deck.push_back({1,0});
    for(int i=0;i<shotguns;++i) deck.push_back({2,0});
    return deck;
}

// ------------------ Level core ------------------
struct GameStats { bool win=false; int lives=0; int moves=0; int mismatches=0; float seconds=0.f; };

// drain all queued clues into the visible journal
inline void drainClues(queue<string>& q, vector<string>& journal){
    while(!q.empty()){ journal.push_back(q.front()); q.pop(); }
}

pair<bool,int> playLevel(sf::RenderWindow& win, sf::Font& uiFont, sf::Font& /*storyFont*/,
                              int rows, int cols,
                              int hearts, int shotguns,
                              float coldPeriodSeconds,
                              vector<string>& journal,
                              vector<string> hintPool, // copy for this level
                              queue<string>& cluesQ,
                              GameStats& out,
                              sf::Color backColor,
                              const char* backImagePath,
                              bool pairColorByValue)
{
    float cardW = (rows==3? 114.f:94.f);
    float cardH = (rows==3? 146.f:126.f);
    float pad   = (rows==3? 18.f:14.f);
    sf::Vector2f gridTop(56.f, 66.f);

    vector<sf::Color> palette = {
        {244,208,111},{133,193,233},{171,235,198},{249,231,159},
        {214,234,248},{250,219,216},{187,222,251},{200,230,201},{255,245,157}
    };
    sf::Texture heartTex;   bool heartOK   = heartTex.loadFromFile(HEART_IMAGE_PATH);
    sf::Texture shotgunTex; bool shotgunOK = shotgunTex.loadFromFile(SHOTGUN_IMAGE_PATH);

    // whole-scene background
    sf::Texture sceneBgTex; bool sceneOK = sceneBgTex.loadFromFile(GAME_BG_PATH);
    sf::Sprite sceneBg;
    if(sceneOK){
        float sx = (float)win.getSize().x / sceneBgTex.getSize().x;
        float sy = (float)win.getSize().y / sceneBgTex.getSize().y;
        float sc = max(sx, sy);
        sceneBg.setTexture(sceneBgTex);
        sceneBg.setScale(sc, sc);
        auto b = sceneBg.getGlobalBounds();
        sceneBg.setPosition((win.getSize().x-b.width)/2.f,(win.getSize().y-b.height)/2.f);
    }

    auto deck = buildDeck(rows,cols,hearts,shotguns);
    mt19937 rng(random_device{}()); shuffle(deck.begin(),deck.end(),rng);

    vector<Card> cards; cards.reserve(rows*cols); int k=0;
    auto getNum=[&](int tp,int val){ return tp==0? val+1 : 0; };
    for(int r=0;r<rows;++r) for(int c=0;c<cols;++c){
        Card cd; auto [tp,val]=deck[k++]; cd.type=tp; cd.value=val;
        cd.rect.setSize({cardW,cardH}); cd.rect.setOrigin(cardW/2,cardH/2);
        float x=gridTop.x + c*(cardW+pad) + cardW/2, y=gridTop.y + r*(cardH+pad) + cardH/2; cd.rect.setPosition(x,y);
        cd.rect.setFillColor(backColor); cd.rect.setOutlineThickness(3); cd.rect.setOutlineColor({15,20,30});
        if (tp==0 && pairColorByValue) cd.frontColor = palette[val % palette.size()];
        else cd.frontColor = palette[(k-1)%palette.size()];
        cd.displayNumber=getNum(tp,val); cards.push_back(cd);
    }

    int lives = INITIAL_LIVES;
    int first=-1, second=-1; bool waiting=false; sf::Clock waitClk;
    int mismatches=0, moves=0;

    stack<int> matchStack;      // consecutive matches for peek
    stack<int> mismatchStack;   // consecutive mismatches for -1 life / swap-two

    bool peekActive=false; int pkA=-1, pkB=-1; sf::Clock pkClk; const float PK_TIME=0.9f;

    sf::Clock coldClk; bool coldBlink=false; float coldBlinkT=0.f;
    sf::Clock levelClk;
    sf::Clock frameClk;

    static bool sgLoaded=false; if(!sgLoaded){ if(audio.shotgunBuf.loadFromFile(SHOTGUN_SFX_PATH)){ audio.shotgun.setBuffer(audio.shotgunBuf); audio.shotgun.setVolume(85.f);} sgLoaded=true; }

    ScrollState jState;

    auto lifeOut = [&](void)->bool{
        // LIFE-OUT: immediate fail
        if(lives <= 0){
            out.win=false; out.lives=0; out.mismatches=mismatches; out.moves=moves; out.seconds=levelClk.getElapsedTime().asSeconds();
            return true;
        }
        return false;
    };

    auto onSuccessfulPair = [&](){
        if(!hintPool.empty()){ cluesQ.push(hintPool.front()); hintPool.erase(hintPool.begin()); }
        while(!mismatchStack.empty()) mismatchStack.pop();
        matchStack.push(1);
        if ((int)matchStack.size() >= 2 && !peekActive){
            vector<int> idx; for(int j=0;j<(int)cards.size();++j) if(cards[j].type==0 && !cards[j].matched && !cards[j].faceUp) idx.push_back(j);
            if ((int)idx.size() >= 2){
                shuffle(idx.begin(), idx.end(), mt19937(random_device{}()));
                pkA = idx[0]; pkB = idx[1];
                cards[pkA].faceUp = cards[pkB].faceUp = true;
                pkClk.restart(); peekActive = true; pushToast("Peek bonus");
            }
            while(!matchStack.empty()) matchStack.pop();
        }
    };

    while(win.isOpen()){
        float dt = frameClk.restart().asSeconds();
        if (dt <= 0.f) dt = 1.f/60.f;
        glowT += dt;

        // drain any newly queued clues into the visible journal
        drainClues(cluesQ, journal);

        sf::Event ev;
        while (win.pollEvent(ev)){
            if (ev.type==sf::Event::Closed){ win.close(); out={false,lives,moves,mismatches,levelClk.getElapsedTime().asSeconds()}; return {false,lives}; }
            if (ev.type==sf::Event::MouseWheelScrolled){
                sf::FloatRect jRect(520,46,368,508);
                sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
                if (jRect.contains(m)){
                    float viewH = 508-56.f;
                    jState.scroll = clamp(jState.scroll - ev.mouseWheelScroll.delta*30.f, 0.f, max(0.f, jState.contentH - viewH));
                }
            }
            if (ev.type==sf::Event::MouseButtonPressed && ev.mouseButton.button==sf::Mouse::Left
                && !peekActive && !waiting)
            {
                sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
                for(int i=0;i<(int)cards.size();++i){
                    auto &c=cards[i]; if(c.matched||c.flipping||c.faceUp) continue;
                    if(!c.rect.getGlobalBounds().contains(m)) continue;

                    // specials
                    if (c.type==1 || c.type==2){
                        if (first!=-1){ cards[first].flipping=true; cards[first].targetUp=false; cards[first].flipT=0.f; first=-1; }
                        c.flipping=true; c.targetUp=true; c.flipT=0.f;
                        if(c.type==1){
                            lives+=1; pushToast("+1 life");
                            if(!hintPool.empty()){ cluesQ.push(hintPool.front()); hintPool.erase(hintPool.begin()); }
                        } else if(c.type==2){
                            lives=max(0,lives-1); pushToast("-1 life");
                            if (audio.shotgunBuf.getSampleCount()>0) audio.shotgun.play();
                            if(lifeOut()) return {false,lives}; // LIFE-OUT: immediate fail
                        }
                        c.matched=true;
                        while(!matchStack.empty()) matchStack.pop();
                        while(!mismatchStack.empty()) mismatchStack.pop();
                        break;
                    }

                    // normal
                    if (c.type==0){
                        c.flipping=true; c.targetUp=true; c.flipT=0.f;
                        if(first==-1) first=i;
                        else if(second==-1 && i!=first){ second=i; waiting=true; waitClk.restart(); }
                        break;
                    }
                }
            }
        }

        // cold trail tick
        if (coldClk.getElapsedTime().asSeconds()>=coldPeriodSeconds){
            lives = max(0, lives-1); coldClk.restart(); coldBlink=true; coldBlinkT=1.4f; pushToast("Cold trail -1");
            if(lifeOut()) return {false,lives}; // LIFE-OUT: immediate fail
        }
        if (coldBlink){ coldBlinkT-=dt; if(coldBlinkT<=0) coldBlink=false; }

        // peek resolve
        if (peekActive && pkClk.getElapsedTime().asSeconds()>PK_TIME){
            if (pkA!=-1 && pkB!=-1){
                for(int id: {pkA,pkB}){ cards[id].flipping=true; cards[id].targetUp=false; cards[id].flipT=0.f; }
            }
            pkA=pkB=-1; peekActive=false;
        }

        // flip anim
        const float FLIP_TIME=0.22f;
        for(auto& c: cards){
            if(!c.flipping) continue; c.flipT += dt/FLIP_TIME; float t=min(c.flipT,1.f);
            float e = easeInOutCubic(t);
            float sx = (e<0.5f)? (1.f-2.f*e) : (2.f*(e-0.5f)); if(t>=0.5f) c.faceUp=c.targetUp;
            c.rect.setScale(max(0.01f,sx), 1.f); if(t>=1.f){ c.flipping=false; c.rect.setScale(1.f,1.f); }
        }

        // pair check
        if (!peekActive && waiting && waitClk.getElapsedTime().asSeconds()>0.5f){
            if(first!=-1 && second!=-1){
                auto &A=cards[first], &B=cards[second];
                bool ok=(A.type==0 && B.type==0 && A.value==B.value);
                if(ok){
                    A.matched=B.matched=true;
                    onSuccessfulPair();
                } else {
                    // mismatch handling via STACK
                    A.flipping=B.flipping=true; A.targetUp=B.targetUp=false; A.flipT=B.flipT=0.f;
                    mismatches++;
                    mismatchStack.push(1);

                    if((int)mismatchStack.size()==2){
                        lives = max(0, lives-1);
                        pushToast("Two mistakes (-1 life)");
                        if(lifeOut()) return {false,lives}; // LIFE-OUT: immediate fail
                    }
                    if((int)mismatchStack.size()>=3){
                        swapTwoHidden(cards, rng);     // instant, no animation
                        pushToast("Board sleight: two hidden cards swapped");
                        while(!mismatchStack.empty()) mismatchStack.pop(); // reset streak
                    }
                    while(!matchStack.empty()) matchStack.pop();
                }
            }
            first=second=-1; waiting=false; moves++;
        }

        // win?
        bool done=true; for(auto& c: cards) if((c.type==0||c.type==1||c.type==2) && !c.matched){ done=false; break; }
        if (done){
            out.win=true; out.lives=lives; out.mismatches=mismatches; out.moves=moves; out.seconds=levelClk.getElapsedTime().asSeconds();
            return {true,lives};
        }

        // draw background photo (or fallback gradient)
        sf::Color top(28,40,70), bot(10,16,28); // slightly cooler palette
        if(lives<=3){ top={90,40,40}; bot={40,18,18}; }
        else if(lives<=5){ top={64,34,96}; bot={24,12,48}; }

        win.clear();
        if(sceneOK) { win.draw(sceneBg); } else { drawGradient(win, top, bot); }

        // draw cards
        for(auto& c: cards){
            sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
            bool hov = (!done && !peekActive && !waiting && c.rect.getGlobalBounds().contains(m));

            if(hov){
                sf::RectangleShape sh = c.rect; sh.setFillColor(sf::Color(0,0,0,60));
                sh.setPosition(c.rect.getPosition()+sf::Vector2f(2.f,4.f));
                win.draw(sh);
            }

            c.rect.setOutlineColor(hov? pulse(glowT) : sf::Color(15,20,30));
            c.rect.setFillColor((c.faceUp||c.matched)? c.frontColor : backColor);

            sf::Vector2f pos=c.rect.getPosition(); float lift=hov? -5.f:0.f; c.rect.setPosition(pos.x,pos.y+lift);
            win.draw(c.rect);

            // card backs: full photo
            if(!(c.faceUp||c.matched) && backImagePath){
                static sf::Texture backTex; static bool loaded=false; static string lastPath;
                if(!loaded || lastPath!=backImagePath){ loaded=backTex.loadFromFile(backImagePath); lastPath=backImagePath; }
                if(loaded){
                    sf::Sprite s(backTex);
                    float sx=(c.rect.getSize().x)/backTex.getSize().x, sy=(c.rect.getSize().y)/backTex.getSize().y;
                    s.setScale(sx,sy); auto b=s.getLocalBounds(); s.setOrigin(b.width/2.f,b.height/2.f);
                    s.setPosition(c.rect.getPosition()); win.draw(s);
                }
            }

            if (c.faceUp||c.matched){
                if (c.type==1 && heartOK){
                    sf::Sprite s(heartTex); float sx=(c.rect.getSize().x)/heartTex.getSize().x, sy=(c.rect.getSize().y)/heartTex.getSize().y;
                    s.setScale(sx,sy); auto b=s.getLocalBounds(); s.setOrigin(b.width/2.f,b.height/2.f); s.setPosition(c.rect.getPosition()); win.draw(s);
                } else if (c.type==2 && shotgunOK){
                    sf::Sprite s(shotgunTex); float sx=(c.rect.getSize().x)/shotgunTex.getSize().x, sy=(c.rect.getSize().y)/shotgunTex.getSize().y;
                    s.setScale(sx,sy); auto b=s.getLocalBounds(); s.setOrigin(b.width/2.f,b.height/2.f); s.setPosition(c.rect.getPosition()); win.draw(s);
                } else if (c.type==0 && c.displayNumber>0){
                    sf::Text t(to_string(c.displayNumber), uiFont, 30); auto b=t.getLocalBounds();
                    t.setOrigin(b.left+b.width/2.f,b.top+b.height/2.f); t.setFillColor(sf::Color(0,0,0,230)); t.setPosition(c.rect.getPosition()); win.draw(t);
                }
            }
            c.rect.setPosition(c.rect.getPosition().x, c.rect.getPosition().y - lift);
        }

        // HUD
        string hudText="Lives: "+to_string(lives); if(coldBlink) hudText+="   (Cold trail -1)";
        sf::Text hudT("",uiFont,22); hudT.setFillColor(sf::Color::White); setUtf8(hudT, hudText); hudT.setPosition(20.f, win.getSize().y-36.f); win.draw(hudT);

        // JOURNAL
        drawJournalPanel(win, uiFont, {520,46}, {368,508}, journal, jState, 21, JOURNAL_BG_IMAGE_PATH);

        // Toasts + overlays
        updateAndDrawToasts(win, uiFont, dt);
        static mt19937 grainRng2(random_device{}()); drawGrain(win, grainRng2, 110);
        drawVignette(win, 64, 96);
        win.display();
    }

    out={false,lives,moves,mismatches,0.f};
    return {false,lives};
}

// ------------------ INTRO ------------------
bool showIntro(sf::RenderWindow& win, sf::Font& font){
    sf::Texture tex; 
    bool hasBg = tex.loadFromFile(INTRO_IMAGE_PATH);

    sf::Text title("", font, 46);
    title.setFillColor(sf::Color(15,15,25));
    title.setOutlineColor(sf::Color(255,245,200));
    title.setOutlineThickness(3.f);
    setUtf8(title, "OBSIDIAN ROSE"); // shorter title fits

    sf::Text prompt("", font, 20);
    prompt.setFillColor(sf::Color(245,245,245));
    prompt.setOutlineThickness(2.f);
    prompt.setOutlineColor(sf::Color(0,0,0,150));
    setUtf8(prompt, "Press any key to continue");

    sf::Sprite sp;
    if(hasBg){
        tex.setSmooth(false);
        sp.setTexture(tex);
        float sx = (float)win.getSize().x / tex.getSize().x;
        float sy = (float)win.getSize().y / tex.getSize().y;
        float scale = max(sx, sy);
        sp.setScale(scale, scale);
        auto b = sp.getGlobalBounds();
        sp.setPosition((win.getSize().x - b.width)/2.f, (win.getSize().y - b.height)/2.f);
    }

    while (win.isOpen()){
        sf::Event e; while (win.pollEvent(e)){
            if (e.type==sf::Event::Closed){ win.close(); return false; }
            if (e.type==sf::Event::KeyPressed || e.type==sf::Event::MouseButtonPressed) return true;
        }
        win.clear(); 
        if(hasBg) win.draw(sp); else drawGradient(win,{8,8,24},{2,2,10});
        drawCentered(win, title, 120.f); 
        drawCentered(win, prompt, (float)win.getSize().y-60.f);
        drawVignette(win, 70, 100);
        win.display();
    }
    return false;
}

// ------------------ Prologue & cast ------------------
bool showPrologue(sf::RenderWindow& win, sf::Font& uiFont, sf::Font& storyFont){
    sf::Texture bgTex; bgTex.loadFromFile(STORY_BG_PATH);
    sf::Sprite bg; float base=1.f;
    if (bgTex.getSize().x>0){
        bg.setTexture(bgTex);
        float sx=win.getSize().x/(float)bgTex.getSize().x, sy=win.getSize().y/(float)bgTex.getSize().y;
        base=max(sx,sy); bg.setScale(base,base);
    }

    sf::Text title("", uiFont, 30); title.setFillColor({255,235,150}); setUtf8(title, "PROLOGUE — THE OBSIDIAN ROSE");
    sf::Text body("", storyFont, 22); body.setFillColor({230,230,240});
    sf::Text prompt("", uiFont, 20); prompt.setFillColor({220,220,230}); setUtf8(prompt, "Press SPACE to continue");

    const string full = PROLOGUE_TEXT;
    float acc=0.f; size_t shown=0; const float cps=13.f; // slower reveal
    sf::Clock clk;

    while (win.isOpen()){
        float dt=clk.restart().asSeconds();
        acc+=dt*cps; size_t want=(size_t)min<float>((float)full.size(), acc);
        if (want>shown) shown=want;

        sf::Text meas("", storyFont, 22);
        auto wrap = [&](const string& src, float maxW){
            vector<string> lines; istringstream iss(src); string w,line;
            while(iss>>w){
                string trial=line.empty()?w:line+" "+w;
                setUtf8(meas, trial);
                if(meas.getLocalBounds().width>maxW){ if(!line.empty()) lines.push_back(line); line=w; }
                else line=trial;
            }
            if(!line.empty()) lines.push_back(line);
            if(lines.empty()) lines.push_back("");
            return lines;
        };
        auto lines = wrap(full.substr(0, shown), 720.f);
        string joined; for(auto& l:lines) joined+=l+"\n";
        setUtf8(body, joined);

        sf::Event e; while (win.pollEvent(e)){
            if (e.type==sf::Event::Closed){ win.close(); return false; }
            if (e.type==sf::Event::KeyPressed && e.key.code==sf::Keyboard::Space) goto CAST_SCREEN;
        }

        win.clear();
        if (bgTex.getSize().x>0){
            auto b=bg.getGlobalBounds();
            bg.setPosition((win.getSize().x-b.width)/2.f,(win.getSize().y-b.height)/2.f);
            win.draw(bg);
            sf::RectangleShape ov({(float)win.getSize().x,(float)win.getSize().y}); ov.setFillColor({0,0,0,180}); win.draw(ov);
        } else {
            drawGradient(win,{15,15,40},{3,3,15});
        }
        drawCentered(win, title, 90.f);
        body.setPosition(60.f, 140.f); win.draw(body);
        drawCentered(win, prompt, (float)win.getSize().y-60.f);
        drawVignette(win, 70, 90);
        win.display();
    }
    return false;

CAST_SCREEN: ;
    mt19937 rng(random_device{}());
    uniform_real_distribution<float> dx(0,(float)win.getSize().x), dy(0,(float)win.getSize().y), dr(0.6f,1.8f), dv(-0.15f,0.15f);
    vector<Star> stars(120);
    for(auto& s:stars){ s.p={dx(rng),dy(rng)}; s.v={dv(rng), dv(rng)}; s.r=dr(rng); s.a=200.f; }

    auto B = bios(); size_t idx=0;
    sf::Text name("",uiFont,28), role("",uiFont,20), note("",storyFont,20);
    name.setFillColor({255,235,150}); role.setFillColor({210,220,240}); note.setFillColor({220,230,255});

    while (win.isOpen()){
        sf::Event e; while(win.pollEvent(e)){
            if(e.type==sf::Event::Closed){ win.close(); return false; }
            if(e.type==sf::Event::KeyPressed && e.key.code==sf::Keyboard::Space){ idx++; if(idx>=B.size()) return true; }
        }
        win.clear(); drawGradient(win,{6,8,20},{1,1,6}); drawStars(win, stars);

        sf::Text t("", uiFont, 28); t.setFillColor({255,235,150}); setUtf8(t, "CAST"); drawCentered(win,t,80);
        setUtf8(name, B[idx].name); drawCentered(win,name,150);
        setUtf8(role, B[idx].role); drawCentered(win,role,190);

        sf::Text meas("",storyFont,20);
        auto wrap = [&](const string& src, float maxW){
            vector<string> lines; istringstream iss(src); string w,line;
            while(iss>>w){
                string trial=line.empty()?w:line+" "+w;
                setUtf8(meas, trial);
                if(meas.getLocalBounds().width>maxW){ if(!line.empty()) lines.push_back(line); line=w; }
                else line=trial;
            }
            if(!line.empty()) lines.push_back(line);
            return lines;
        };
        auto lines=wrap(B[idx].note, 720.f);
        string joined; for(auto& l:lines) joined+=l+"\n";
        setUtf8(note, joined); note.setPosition(60.f,240.f); win.draw(note);

        drawVignette(win, 70, 80);
        drawCentered(win, sf::Text(utf8("Press SPACE to continue"), uiFont, 20), (float)win.getSize().y-60.f);
        win.display();
    }
    return false;
}
void showInterlude(sf::RenderWindow& win, sf::Font& uiFont, sf::Font& storyFont, const string& text, const char* prompt="Press SPACE to continue"){
    // Load interlude bg
    sf::Texture bgTex;
    bool hasBg = bgTex.loadFromFile(INTERLUDE_BG_PATH);
    sf::Sprite bg;
    if(hasBg){
        float sx = (float)win.getSize().x / bgTex.getSize().x;
        float sy = (float)win.getSize().y / bgTex.getSize().y;
        float sc = max(sx, sy);
        bg.setTexture(bgTex);
        bg.setScale(sc, sc);
        auto b = bg.getGlobalBounds();
        // FIX: win->getSize().y  --->  win.getSize().y
        bg.setPosition((win.getSize().x - b.width)/2.f, (win.getSize().y - b.height)/2.f);
    }

    sf::Text title("", uiFont, 28); title.setFillColor({255,235,150}); setUtf8(title, "INTERLUDE");
    sf::Text body("", storyFont, 20); body.setFillColor({230,230,240}); body.setPosition(60,140);
    auto wrap = [&](const string& src, float maxW){
        vector<string> lines; sf::Text meas("",storyFont,20); istringstream iss(src); string w,line;
        while(iss>>w){
            string trial=line.empty()?w:line+" "+w;
            setUtf8(meas, trial);
            if(meas.getLocalBounds().width>maxW){ if(!line.empty()) lines.push_back(line); line=w; }
            else line=trial;
        }
        if(!line.empty()) lines.push_back(line); return lines;
    };
    auto lines = wrap(text, 720.f); string joined; for(auto&s:lines) joined+=s+"\n"; setUtf8(body, joined);
    sf::Text pr("",uiFont,20); pr.setFillColor({220,220,230}); setUtf8(pr, prompt);

    while (win.isOpen()){
        sf::Event e; while (win.pollEvent(e)){
            if(e.type==sf::Event::Closed){ win.close(); return; }
            if(e.type==sf::Event::KeyPressed && (e.key.code==sf::Keyboard::Space || e.key.code==sf::Keyboard::Escape)) return;
            if(e.type==sf::Event::MouseButtonPressed) return;
        }

        win.clear();
        if(hasBg){
            win.draw(bg);
            // readability veil
            sf::RectangleShape veil({(float)win.getSize().x,(float)win.getSize().y});
            veil.setFillColor(sf::Color(0,0,0,150));
            win.draw(veil);
        } else {
            drawGradient(win,{12,14,28},{3,3,12});
        }
        drawCentered(win,title,90);
        win.draw(body);
        drawCentered(win,pr,(float)win.getSize().y-60);
        drawVignette(win,64,90);
        win.display();
    }
}


// ------------------ Suspect choice (3 vertically, journal on right, custom colors & bg) ------------------
bool chooseSuspect3(sf::RenderWindow& win, sf::Font& uiFont, sf::Font& storyFont, const vector<string>& journal, const string& killerName){
    // Background image for suspects screen
    sf::Texture bgTex; bool hasBg = bgTex.loadFromFile(SUSPECT_BG_PATH);
    sf::Sprite bg;
    if(hasBg){
        float sx = (float)win.getSize().x / bgTex.getSize().x;
        float sy = (float)win.getSize().y / bgTex.getSize().y;
        float sc = max(sx, sy);
        bg.setTexture(bgTex);
        bg.setScale(sc, sc);
        auto b = bg.getGlobalBounds();
        bg.setPosition((win.getSize().x - b.width)/2.f, (win.getSize().y - b.height)/2.f);
    }

    sf::Text title("", uiFont, 26); title.setFillColor({255,235,150}); setUtf8(title, "Choose the culprit");
    sf::Text prompt("", uiFont, 18); prompt.setFillColor({220,220,230}); prompt.setPosition(40,80); setUtf8(prompt, "Review your journal, then choose.");

    // Three vertical buttons on the left with distinct colors
    sf::RectangleShape B1({280,70}), B2({280,70}), B3({280,70});
    B1.setPosition(40,130); B2.setPosition(40,220); B3.setPosition(40,310);

    // Serena: teal; Marcus: amber; Evelyn: plum
    sf::Color sN(  0,139,139), sH(  0,170,170);
    sf::Color mN(193,134,  0), mH(223,164, 20);
    sf::Color eN(128, 90,165), eH(160,115,200);

    B1.setFillColor(sN); B2.setFillColor(mN); B3.setFillColor(eN);
    for(auto*b:{&B1,&B2,&B3}){ b->setOutlineThickness(3); b->setOutlineColor({225,225,240}); }

    sf::Text T1("",uiFont,22), T2("",uiFont,22), T3("",uiFont,22);
    setUtf8(T1,"Dr. Serena Bellwether");
    setUtf8(T2,"Marcus Thorne");
    setUtf8(T3,"Evelyn Reed");
    for(auto*t:{&T1,&T2,&T3}) t->setFillColor(sf::Color(255,255,255));

    auto center=[&](sf::Text& tx, const sf::RectangleShape& r){ auto b=tx.getLocalBounds(); tx.setOrigin(b.left+b.width/2.f,b.top+b.height/2.f); auto p=r.getPosition(), s=r.getSize(); tx.setPosition(p.x+s.x/2.f, p.y+s.y/2.f); };
    center(T1,B1); center(T2,B2); center(T3,B3);

    // Journal on the right (moved so it never covers buttons)
    ScrollState st;

    while (win.isOpen()){
        sf::Event e; while (win.pollEvent(e)){
            if(e.type==sf::Event::Closed){ win.close(); return false; }
            if(e.type==sf::Event::MouseWheelScrolled){
                sf::FloatRect jr(520,100,340,420);
                sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
                if (jr.contains(m)){
                    float viewH = 420-56.f;
                    st.scroll = clamp(st.scroll - e.mouseWheelScroll.delta*30.f, 0.f, max(0.f, st.contentH - viewH));
                }
            }
            if(e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){
                sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
                if(B1.getGlobalBounds().contains(m)) return (string("Serena")==killerName);
                if(B2.getGlobalBounds().contains(m)) return (string("Marcus")==killerName);
                if(B3.getGlobalBounds().contains(m)) return (string("Evelyn")==killerName);
            }
        }

        // hover colors
        sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
        B1.setFillColor(B1.getGlobalBounds().contains(m)?sH:sN);
        B2.setFillColor(B2.getGlobalBounds().contains(m)?mH:mN);
        B3.setFillColor(B3.getGlobalBounds().contains(m)?eH:eN);

        win.clear();
        if(hasBg){
            win.draw(bg);
            // Dark veil for readability and cooler tone background change
            sf::RectangleShape veil({(float)win.getSize().x,(float)win.getSize().y});
            veil.setFillColor(sf::Color(10,12,24,180));
            win.draw(veil);
        } else {
            drawGradient(win, {12,18,34}, {4,6,14}); // darker blue fallback
        }

        drawCentered(win,title,40); win.draw(prompt);
        win.draw(B1); win.draw(T1);
        win.draw(B2); win.draw(T2);
        win.draw(B3); win.draw(T3);

        drawJournalPanel(win, uiFont, {520,100}, {340,420}, journal, st, 20, JOURNAL_BG_IMAGE_PATH);
        drawVignette(win,64,90);
        win.display();
    }
    return false;
}

// ------------------ How To ------------------
void showHowTo(sf::RenderWindow& win, sf::Font& uiFont, sf::Font& storyFont){
    sf::Text t("", storyFont,18);
    setUtf8(t,
        "How to Play\n\n"
        "- Match pairs to recover clues; every match adds a Journal entry.\n"
        "- Heart: +1 life.  SG (Shotgun): -1 life (gunshot).\n"
        "- Streaks: after 2 consecutive mismatches => -1 life; after the 3rd => two hidden cards swap.\n"
        "- Cold trail: wait too long and you lose a life.\n\n"
        "Level 1: 3x3.  Level 2: 4x4.\n"
        "- Bonus: after 2 consecutive correct matches, you briefly peek 2 random cards.\n\n"
        "Press ESC, any key, or Back to return.");
    t.setFillColor(sf::Color::White); t.setPosition(40,140);

    sf::Text title("", uiFont, 26); title.setFillColor({255,235,150}); setUtf8(title,"HOW TO PLAY");

    // Back button
    sf::RectangleShape backBtn({160,46}); backBtn.setPosition(40, 560);
    sf::Color bC(90,110,150), bH(120,145,185);
    backBtn.setFillColor(bC); backBtn.setOutlineThickness(3); backBtn.setOutlineColor({200,200,230});
    sf::Text backTx("", uiFont, 20); setUtf8(backTx, "Back");
    auto center=[&](sf::Text& tx, const sf::RectangleShape& r){ auto b=tx.getLocalBounds(); tx.setOrigin(b.left+b.width/2.f,b.top+b.height/2.f); auto p=r.getPosition(), s=r.getSize(); tx.setPosition(p.x+s.x/2.f, p.y+s.y/2.f); };
    center(backTx, backBtn);

    while(win.isOpen()){
        sf::Event e; 
        while(win.pollEvent(e)){
            if(e.type==sf::Event::Closed){ win.close(); return; }
            if(e.type==sf::Event::KeyPressed){ return; }
            if(e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){
                sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
                if(backBtn.getGlobalBounds().contains(m)) return;
            }
        }
        sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
        backBtn.setFillColor(backBtn.getGlobalBounds().contains(m)?bH:bC);

        win.clear(); drawGradient(win,{12,14,28},{3,3,12});
        win.draw(title); win.draw(t); 
        win.draw(backBtn); win.draw(backTx);
        drawVignette(win,64,90);
        win.display();
    }
}

// ------------------ Wait message helper ------------------
bool waitMessageScreen(sf::RenderWindow& win, sf::Font& uiFont, const string& text, sf::Color top, sf::Color bottom){
    sf::Text t("", uiFont, 22); setUtf8(t, text); t.setFillColor(sf::Color::White); t.setPosition(40,220);
    while (win.isOpen()){
        sf::Event e; while (win.pollEvent(e)){
            if(e.type==sf::Event::Closed){ win.close(); return false; }
            if(e.type==sf::Event::KeyPressed || e.type==sf::Event::MouseButtonPressed){ return true; }
        }
        win.clear(); drawGradient(win, top, bottom); win.draw(t); drawVignette(win,64,90); win.display();
    }
    return false;
}

// ------------------ Heap leaderboard (recursive) ------------------
inline int left(int i){ return 2*i+1; }
inline int right(int i){ return 2*i+2; }

// "better" means ranks higher on the board
bool better(const LeaderRecord& a, const LeaderRecord& b){
    if(a.won != b.won) return a.won && !b.won;         // won > lost
    if(a.seconds != b.seconds) return a.seconds < b.seconds; // lower time is better
    return a.date < b.date; // tie-breaker
}

void heapifyDownRec(vector<LeaderRecord>& a, int n, int i){
    int largest = i;
    int L = left(i), R = right(i);
    if(L < n && better(a[L], a[largest])) largest = L;
    if(R < n && better(a[R], a[largest])) largest = R;
    if(largest != i){
        swap(a[i], a[largest]);
        heapifyDownRec(a, n, largest);
    }
}

void buildMaxHeapRec(vector<LeaderRecord>& a){
    for(int i=(int)a.size()/2 - 1; i>=0; --i) heapifyDownRec(a, (int)a.size(), i);
}

// Recursive wrapper to perform heap sort (max to end; result worst..best)
void _heapSortEndRec(vector<LeaderRecord>& a, int end){
    if(end <= 1) return;
    swap(a[0], a[end-1]);
    heapifyDownRec(a, end-1, 0);
    _heapSortEndRec(a, end-1);
}
void heapSortRecordsRecursive(vector<LeaderRecord>& a){
    if(a.empty()) return;
    buildMaxHeapRec(a);
    _heapSortEndRec(a, (int)a.size());
    // Result: a[0..n-1] is sorted ascending by "better" (worst..best). Show from back.
}

// ------------------ MAIN ------------------
int main(){
    sf::RenderWindow win(sf::VideoMode(900,650),"Detective Eclipse: Obsidian Rose", sf::Style::Titlebar|sf::Style::Close|sf::Style::Resize);
    win.setFramerateLimit(60);

    sf::Font uiFont;    if(!uiFont.loadFromFile(UI_FONT_PATH))   { cerr<<"Install fonts-dejavu-core\n"; return 1; }
    sf::Font storyFont; if(!storyFont.loadFromFile(STORY_FONT_PATH)){ storyFont = uiFont; }

    if(!showIntro(win,uiFont)) return 0;

    while(win.isOpen()){
        // MENU
        sf::Text title("", uiFont, 32); title.setFillColor({255,235,180}); setUtf8(title,"DETECTIVE ECLIPSE");

        sf::RectangleShape bStart({280,60}), bHow({280,60}), bBoard({280,60}), bQuit({280,60});
        float bx=(900-280)/2.f; bStart.setPosition(bx,230); bHow.setPosition(bx,320); bBoard.setPosition(bx,410); bQuit.setPosition(bx,500);

        sf::Color startC(70,120,90),  startH(95,155,120);
        sf::Color howC(70,95,150),    howH(95,125,185);
        sf::Color boardC(110,80,145), boardH(140,105,185);
        sf::Color quitC(150,70,70),   quitH(185,95,95);

        bStart.setFillColor(startC); bHow.setFillColor(howC); bBoard.setFillColor(boardC); bQuit.setFillColor(quitC);
        for(auto*b:{&bStart,&bHow,&bBoard,&bQuit}){ b->setOutlineThickness(3); b->setOutlineColor({200,200,230}); }

        sf::Text tStart("",uiFont,24), tHow("",uiFont,24), tBoard("",uiFont,24), tQuit("",uiFont,24);
        setUtf8(tStart,"Start"); setUtf8(tHow,"How to Play"); setUtf8(tBoard,"Leaderboard"); setUtf8(tQuit,"Quit");
        for(auto*t:{&tStart,&tHow,&tBoard,&tQuit}){ t->setFillColor(sf::Color(245,245,245)); }
        auto center=[&](sf::Text&t,const sf::RectangleShape&r){ auto b=t.getLocalBounds(); t.setOrigin(b.left+b.width/2.f,b.top+b.height/2.f); auto p=r.getPosition(), s=r.getSize(); t.setPosition(p.x+s.x/2.f,p.y+s.y/2.f); };
        center(tStart,bStart); center(tHow,bHow); center(tBoard,bBoard); center(tQuit,bQuit);

        int choice=0; bool menu=true;
        while(menu && win.isOpen()){
            sf::Event e; while(win.pollEvent(e)){
                if(e.type==sf::Event::Closed){ win.close(); return 0; }
                if(e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left){
                    sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
                    if(bStart.getGlobalBounds().contains(m)){ choice=1; menu=false; }
                    else if(bHow.getGlobalBounds().contains(m)){ choice=2; menu=false; }
                    else if(bBoard.getGlobalBounds().contains(m)){ choice=3; menu=false; }
                    else if(bQuit.getGlobalBounds().contains(m)){ choice=4; menu=false; }
                }
                if(e.type==sf::Event::KeyPressed){
                    if(e.key.code==sf::Keyboard::Num1) { choice=1; menu=false; }
                    if(e.key.code==sf::Keyboard::Num2) { choice=2; menu=false; }
                    if(e.key.code==sf::Keyboard::Num3) { choice=3; menu=false; }
                    if(e.key.code==sf::Keyboard::Num4 || e.key.code==sf::Keyboard::Escape) { choice=4; menu=false; }
                }
            }
            sf::Vector2f m=win.mapPixelToCoords(sf::Mouse::getPosition(win));
            bStart.setFillColor(bStart.getGlobalBounds().contains(m)?startH:startC);
            bHow.setFillColor(bHow.getGlobalBounds().contains(m)?howH:howC);
            bBoard.setFillColor(bBoard.getGlobalBounds().contains(m)?boardH:boardC);
            bQuit.setFillColor(bQuit.getGlobalBounds().contains(m)?quitH:quitC);

            win.clear(); drawGradient(win,{8,8,24},{2,2,10}); drawCentered(win,title,140);
            win.draw(bStart); win.draw(tStart); win.draw(bHow);   win.draw(tHow);
            win.draw(bBoard); win.draw(tBoard); win.draw(bQuit);  win.draw(tQuit);
            drawVignette(win,64,90);
            win.display();
        }

        if(!win.isOpen()) break;
        if(choice==4) break;
        if(choice==2){ showHowTo(win,uiFont,storyFont); continue; }
        if(choice==3){
            auto all = loadRecords();
            heapSortRecordsRecursive(all); // worst..best
            sf::Text title2("", uiFont, 28); title2.setFillColor({255,235,150}); setUtf8(title2,"LEADERBOARD (Top 10)");
            sf::Text row("",uiFont,18); row.setFillColor({220,230,255});
            while(win.isOpen()){
                sf::Event e; while(win.pollEvent(e)){ 
                    if(e.type==sf::Event::Closed){ win.close(); break; } 
                    if(e.type==sf::Event::KeyPressed||e.type==sf::Event::MouseButtonPressed){ goto EXIT_LEADER; } 
                }
                win.clear(); drawGradient(win,{10,12,26},{3,3,12}); drawCentered(win,title2,80);
                float y=140;
                // print Top 10 from best to worst (array is worst..best after heapsort)
                int printed=0;
                for(int i=(int)all.size()-1; i>=0 && printed<10; --i, ++printed){
                    ostringstream os; 
                    os<<printed+1<<". Time: "<<(int)round(all[i].seconds)<<"s | Result: "<<(all[i].won? "Won":"Lost");
                    setUtf8(row, os.str()); row.setPosition(80,y); win.draw(row); y+=30;
                }
                drawVignette(win,64,90); win.display();
            }
            EXIT_LEADER:
            continue;
        }

        if(!showPrologue(win,uiFont,storyFont)) break;

        // Journal (displayed) + Clues queue (FIFO)
        vector<string> journal;
        queue<string> cluesQ;

        // Level 1 (3x3)
        GameStats s1{};
        auto L1 = playLevel(win, uiFont, storyFont, 3,3, /*hearts*/1, /*SG*/0, /*cold*/30.f,
                            journal, clues_level1(), cluesQ, s1,
                            sf::Color(30, 70, 90), CARD_BACK_IMAGE_PATH, true);
        if(!L1.first){
            saveRecord({s1.seconds, false, nowDate()});
            bool ok = waitMessageScreen(win, uiFont,
                "Result: LOST (board).\nPress any key to return to the menu.",
                {20,8,12}, {5,0,3});
            if(!ok) return 0;
            continue;
        }

        showInterlude(win, uiFont, storyFont,
            "The trail is real now. The stock bet, the soil, the lock timer—someone planned for silence. "
            "You’re close. Go again, and remember faster.");

        // Level 2 (4x4)
        GameStats s2{};
        auto L2 = playLevel(win, uiFont, storyFont, 4,4, /*hearts*/2, /*SG*/2, /*cold*/25.f,
                            journal, clues_level2(), cluesQ, s2,
                            sf::Color(42, 86, 102), CARD_BACK_IMAGE_PATH, true);
        if(!L2.first){
            saveRecord({s1.seconds + s2.seconds, false, nowDate()});
            bool ok = waitMessageScreen(win, uiFont,
                "Result: LOST (board).\nPress any key to return to the menu.",
                {20,8,12}, {5,0,3});
            if(!ok) return 0;
            continue;
        }

        // Suspect guess (killer is Serena)
        {
            string killerName = "Serena";
            bool correct = chooseSuspect3(win, uiFont, storyFont, journal, killerName);
            if (!correct){
                saveRecord({s1.seconds + s2.seconds, false, nowDate()});
                string msg = "Result: LOST (wrong suspect).\n\nThe real culprit was: Dr. Serena Bellwether.\n\nPress any key to return.";
                bool ok = waitMessageScreen(win, uiFont, msg, {20,8,12}, {5,0,3});
                if(!ok) return 0;
                continue;
            }
        }

        // Win summary
        {
            float totalSeconds = s1.seconds + s2.seconds;
            saveRecord({totalSeconds, true, nowDate()});
            ostringstream os; os<<"Result: WON\n\nTime: "<<(int)round(totalSeconds)<<"s\n\nPress any key to return.";
            bool ok = waitMessageScreen(win, uiFont, os.str(), {10,12,28}, {3,3,12});
            if(!ok) return 0;
            continue;
        }
    }
    return 0;
}
